/* 
	Copyright 2010 OpenRTMFP
 
	This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License received along this program for more
	details (or else see http://www.gnu.org/licenses/).

	This file is a part of Cumulus.
*/

#include "RTMFPServer.h"
#include "RTMFP.h"
#include "Handshake.h"
#include "Middle.h"
#include "PacketWriter.h"
#include "Util.h"
#include "Logs.h"
#include "Poco/Format.h"
#include <cstring>


using namespace std;
using namespace Poco;
using namespace Poco::Net;


namespace Cumulus {

class RTMFPManager : private Task, private Startable {
public:
	RTMFPManager(RTMFPServer& server):_server(server),Task(server),Startable("RTMFPManager")  {
		start();
	}
	virtual ~RTMFPManager() {
		stop();
	}
	void run() {
		setPriority(Thread::PRIO_LOW);
		do {
			waitHandle();
		} while(sleep(2000)!=STOP);
	}
private:
	void handle() {
		_server.manage();
	}
	RTMFPServer& _server;
};


RTMFPServer::RTMFPServer(UInt32 threads) : Startable("RTMFPServer"),_pCirrus(NULL),_handshake(*this,*this,*this),_sessions(*this),Handler(threads) {
#ifndef POCO_OS_FAMILY_WINDOWS
//	static const char rnd_seed[] = "string to make the random number generator think it has entropy";
//	RAND_seed(rnd_seed, sizeof(rnd_seed));
#endif
	DEBUG("Id of this RTMFP server : %s",Util::FormatHex(id,ID_SIZE).c_str());
}

RTMFPServer::RTMFPServer(const string& name,UInt32 threads) : Startable(name),_pCirrus(NULL),_handshake(*this,*this,*this),_sessions(*this),Handler(threads) {
#ifndef POCO_OS_FAMILY_WINDOWS
//	static const char rnd_seed[] = "string to make the random number generator think it has entropy";
//	RAND_seed(rnd_seed, sizeof(rnd_seed));
#endif
	DEBUG("Id of this RTMFP server : %s",Util::FormatHex(id,ID_SIZE).c_str());
}

RTMFPServer::~RTMFPServer() {
	stop();
}

void RTMFPServer::start() {
	RTMFPServerParams params;
	start(params);
}

void RTMFPServer::start(RTMFPServerParams& params) {
	if(running()) {
		ERROR("RTMFPServer server is yet running, call stop method before");
		return;
	}
	_host = params.host;
	_port = params.port;
	if(_port==0) {
		ERROR("RTMFPServer port must have a positive value");
		return;
	}
	if(params.pCirrus) {
		_pCirrus = new Target(*params.pCirrus);
		NOTE("RTMFPServer started in man-in-the-middle mode with server %s (unstable debug mode)",_pCirrus->address.toString().c_str());
	}
	_middle = params.middle;
	if(_middle)
		NOTE("RTMFPServer started in man-in-the-middle mode between peers (unstable debug mode)");

	_pSocket = new DatagramSocket();
	 (UInt32&)udpBufferSize = params.udpBufferSize==0 ? _pSocket->getReceiveBufferSize() : params.udpBufferSize;
	_pSocket->setReceiveBufferSize(udpBufferSize);_pSocket->setSendBufferSize(udpBufferSize);
	DEBUG("Socket buffer receving/sending size = %u/%u",udpBufferSize,udpBufferSize);

	(UInt32&)keepAliveServer = params.keepAliveServer<5 ? 5000 : params.keepAliveServer*1000;
	(UInt32&)keepAlivePeer = params.keepAlivePeer<5 ? 5000 : params.keepAlivePeer*1000;

	Startable::start();
	setPriority(params.threadPriority);
}

void RTMFPServer::run() {

	try {
		_pSocket->bind(SocketAddress(_host,_port));
		_mainSockets.add(*_pSocket,*this);

		NOTE("RTMFP server starts on %u port",_port);
		onStart();

		RTMFPManager manager(*this);
		bool terminate=false;
		while(!terminate)
			handle(terminate);

	} catch(Exception& ex) {
		FATAL("RTMFPServer, %s",ex.displayText().c_str());
	} catch (exception& ex) {
		FATAL("RTMFPServer, %s",ex.what());
	} catch (...) {
		FATAL("RTMFPServer, unknown error");
	}

	_mainSockets.remove(*_pSocket);

	// terminate handle
	terminate();
	
	// clean sessions, and send died message if need
	_handshake.clear();
	_sessions.clear();

	// stop receiving and sending engine (it waits the end of sending last session messages)
	poolThreads.clear();

	// close UDP socket
	delete _pSocket;

	sockets.clear();
	_mainSockets.clear();
	_port=0;
	onStop();

	if(_pCirrus) {
		delete _pCirrus;
		_pCirrus = NULL;
	}
	
	NOTE("RTMFP server stops");
}


void RTMFPServer::onError(const Poco::Net::Socket& socket,const std::string& error) {
	WARN("RTMFPServer, %s",error.c_str());
}

void RTMFPServer::onReadable(Socket& socket) {
	// Running on the thread of manager socket
	AutoPtr<RTMFPReceiving> pRTMFPReceiving(new RTMFPReceiving(*this,(DatagramSocket&)socket));
	if(!pRTMFPReceiving->pPacket)
		return;

	if(pRTMFPReceiving->id==0) {
		_handshake.decode(pRTMFPReceiving);
		return;
	}
	ScopedLock<Mutex>  lock(_sessions.mutex);
	Session* pSession = _sessions.find(pRTMFPReceiving->id);
	if(!pSession) {
		WARN("Unknown session %u",pRTMFPReceiving->id);
		return;
	}
	pSession->decode(pRTMFPReceiving);
}

void RTMFPServer::handle(bool& terminate){
	if(sleep()!=STOP)
		giveHandle();
	else
		terminate = true;
}

void RTMFPServer::receive(RTMFPReceiving& rtmfpReceiving) {
	// Process packet
	Session* pSession = NULL;
	if(rtmfpReceiving.id==0) {
		DEBUG("Handshaking");
		pSession = &_handshake;
	} else
		pSession = _sessions.find(rtmfpReceiving.id);
	if(!pSession)
		return; // No warn because can have been deleted during decoding threading process
	if(!pSession->checked) {
		(bool&)pSession->checked = true;
		CookieComputing* pCookieComputing = pSession->peer.object<CookieComputing>();
		_handshake.commitCookie(pCookieComputing->value);
		pCookieComputing->release();
	}
	SocketAddress oldAddress = pSession->peer.address;
	if(pSession->setEndPoint(rtmfpReceiving.socket,rtmfpReceiving.address) && rtmfpReceiving.id!=0)
		_sessions.changeAddress(oldAddress,*pSession);
	pSession->receive(*rtmfpReceiving.pPacket);
}

UInt8 RTMFPServer::p2pHandshake(const string& tag,PacketWriter& response,const SocketAddress& address,const UInt8* peerIdWanted) {

	ServerSession* pSessionWanted = (ServerSession*)_sessions.find(peerIdWanted);

	if(_pCirrus) {
		// Just to make working the man in the middle mode !

		// find the flash client equivalence
		Session* pSession = _sessions.find(address);
		if(!pSession) {
			ERROR("UDP Hole punching error : middle equivalence not found for session wanted");
			return 0;
		}

		PacketWriter& request = ((Middle*)pSession)->handshaker();
		request.write8(0x22);request.write8(0x21);
		request.write8(0x0F);
		request.writeRaw(pSessionWanted ? ((Middle*)pSessionWanted)->middlePeer().id : peerIdWanted,ID_SIZE);
		request.writeRaw(tag);

		((Middle*)pSession)->sendHandshakeToTarget(0x30);
		// no response here!
		return 0;
	}

	
	if(!pSessionWanted) {
		DEBUG("UDP Hole punching : session %s wanted not found",Util::FormatHex(peerIdWanted,ID_SIZE).c_str())
		
		set<string> addresses;
		onRendezVousUnknown(peerIdWanted,addresses);
		if(addresses.empty())
			return 0;
		set<string>::const_iterator it;
		for(it=addresses.begin();it!=addresses.end();++it) {
			try {
				SocketAddress address(*it);
				response.writeAddress(address,it==addresses.begin());
			} catch(Exception& ex) {
				ERROR("Bad redirection address %s, %s",(*it).c_str(),ex.displayText().c_str());
			}
		}
		return 0x71;

	} else if(pSessionWanted->failed()) {
		DEBUG("UDP Hole punching : session wanted is deleting");
		return 0;
	}

	UInt8 result = 0x00;
	if(_middle) {
		if(pSessionWanted->pTarget) {
			HelloAttempt& attempt = _handshake.helloAttempt<HelloAttempt>(tag);
			attempt.pTarget = pSessionWanted->pTarget;
			_handshake.createCookie(response,attempt,tag,"");
			response.writeRaw(&pSessionWanted->pTarget->publicKey[0],pSessionWanted->pTarget->publicKey.size());
			result = 0x70;
		} else
			ERROR("Peer/peer dumped exchange impossible : no corresponding 'Target' with the session wanted");
	}

	
	if(result==0x00) {
		
		/// Udp hole punching normal process
		UInt32 times = pSessionWanted->helloAttempt(tag);
		pSessionWanted->p2pHandshake(address,tag,times,(times>0 || address.host()==pSessionWanted->peer.address.host()) ? _sessions.find(address) : NULL);
		
		bool first=true;
		list<Address>::const_iterator it2;
		for(it2=pSessionWanted->peer.addresses.begin();it2!=pSessionWanted->peer.addresses.end();++it2) {
			const Address& addr = *it2;
			if(addr == address)
				WARN("A client tries to connect to himself (same %s address)",address.toString().c_str());
			response.writeAddress(addr,first);
			DEBUG("P2P address initiator exchange, %s:%u",Util::FormatHex(&addr.host[0],addr.host.size()).c_str(),addr.port);
			first=false;
		}

		result = 0x71;
	}

	return result;

}

Session& RTMFPServer::createSession(const Peer& peer,Cookie& cookie) {

	Target* pTarget=_pCirrus;

	if(_middle) {
		if(!cookie.pTarget) {
			cookie.pTarget = new Target(peer.address,&cookie);
			memcpy((UInt8*)cookie.pTarget->peerId,peer.id,ID_SIZE);
			memcpy((UInt8*)peer.id,cookie.pTarget->id,ID_SIZE);
			NOTE("Mode 'man in the middle' : to connect to peer '%s' use the id :\n%s",Util::FormatHex(cookie.pTarget->peerId,ID_SIZE).c_str(),Util::FormatHex(cookie.pTarget->id,ID_SIZE).c_str());
		} else
			pTarget = cookie.pTarget;
	}

	ServerSession* pSession;
	if(pTarget) {
		pSession = new Middle(_sessions.nextId(),cookie.farId,peer,cookie.decryptKey(),cookie.encryptKey(),*this,_sessions,*pTarget);
		if(_pCirrus==pTarget)
			pSession->pTarget = cookie.pTarget;
		DEBUG("Wait cirrus handshaking");
		pSession->manage(); // to wait the cirrus handshake
	} else {
		pSession = new ServerSession(_sessions.nextId(),cookie.farId,peer,cookie.decryptKey(),cookie.encryptKey(),*this);
		pSession->pTarget = cookie.pTarget;
	}

	_sessions.add(pSession);

	return *pSession;
}

void RTMFPServer::destroySession(Session& session) {
	if(!session.checked) {
		(bool&)session.checked = true;
		session.peer.object<CookieComputing>()->release();
	}
}

void RTMFPServer::manage() {
	_handshake.manage();
	_sessions.manage();
}



} // namespace Cumulus
