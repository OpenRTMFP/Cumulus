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
#include "string.h"


using namespace std;
using namespace Poco;
using namespace Poco::Net;


namespace Cumulus {

RTMFPServer::RTMFPServer(UInt8 keepAliveServer,UInt8 keepAlivePeer) : _handler(keepAliveServer,keepAlivePeer,NULL),_terminate(false),_pCirrus(NULL),_handshake(*this,_socket,_handler),_nextIdSession(0) {
#ifndef _WIN32
//	static const char rnd_seed[] = "string to make the random number generator think it has entropy";
//	RAND_seed(rnd_seed, sizeof(rnd_seed));
#endif
}


RTMFPServer::RTMFPServer(ClientHandler& clientHandler,UInt8 keepAliveServer,UInt8 keepAlivePeer) : _handler(keepAliveServer,keepAlivePeer,&clientHandler),_terminate(false),_pCirrus(NULL),_handshake(*this,_socket,_handler),_nextIdSession(0) {
#ifndef _WIN32
//	static const char rnd_seed[] = "string to make the random number generator think it has entropy";
//	RAND_seed(rnd_seed, sizeof(rnd_seed));
#endif
}

RTMFPServer::~RTMFPServer() {
	stop();
}

Session* RTMFPServer::findSession(UInt32 id) {
 
	// Id session can't be egal to 0 (it's reserved to Handshake)
	if(id==0) {
		DEBUG("Handshaking");
		return &_handshake;
	}
	Session* pSession = _sessions.find(id);
	if(pSession) {
		DEBUG("Session d'identification '%u'",id);
		if(!pSession->checked)
			_handshake.commitCookie(*pSession);
		return pSession;
	}

	WARN("Unknown session '%u'",id);
	return NULL;
}

void RTMFPServer::start(RTMFPServerParams& params) {
	ScopedLock<FastMutex> lock(_mutex);
	if(_mainThread.isRunning()) {
		ERROR("RTMFPServer server is yet running, call stop method before");
		return;
	}
	_port = params.port;
	_freqManage = 2000000; // 2 sec by default
	if(params.pCirrus) {
		_pCirrus = new Target(*params.pCirrus);
		_freqManage = 0; // no waiting, direct process in the middle case!
	}
	_middle = params.middle;
	_terminate = false;
	_mainThread.start(*this);
	
}

void RTMFPServer::stop() {
	ScopedLock<FastMutex> lock(_mutex);
	_terminate = true;
	if(_mainThread.isRunning())
		_mainThread.join();
	if(_pCirrus) {
		delete _pCirrus;
		_pCirrus = NULL;
	}
}

void RTMFPServer::run() {
	SetThreadName("RTMFPServer");
	SocketAddress address("0.0.0.0",_port);
	_socket.bind(address,true);
	
	SocketAddress sender;
	UInt8 buff[PACKETRECV_SIZE];
	int size = 0;
	Timespan span(250000);

	NOTE("RTMFP server starts on %hu port",_port);

	while(!_terminate) {

		manage();

		try {
			if (!_socket.poll(span, Socket::SELECT_READ))
				continue;
			size = _socket.receiveFrom(buff,sizeof(buff),sender);
		} catch(Exception& ex) {
			WARN("Main socket reception : %s",ex.displayText().c_str());
			_socket.close();
			_socket.bind(address,true);
			continue;
		}

		DEBUG("Sender : %s",sender.toString().c_str());

		// A very small test port protocol (echo one byte)
		if(size==1) {
			_socket.sendTo(buff,1,sender);
			continue;
		}

		PacketReader packet(buff,size);
		if(packet.available()<RTMFP_MIN_PACKET_SIZE) {
			ERROR("Invalid packet");
			continue;
		}

		UInt32 idSession = RTMFP::Unpack(packet);
		Session* pSession = this->findSession(idSession);

		if(!pSession)
			continue;

		if(!pSession->decode(packet,sender)) {
			Logs::Dump(packet,"Packet decrypted:");
			ERROR("Decrypt error");
			continue;
		}
	
		Logs::Dump(packet,"Request:");

		pSession->packetHandler(packet);

	}

	INFO("RTMFP server stopping");
	
	_sessions.clear();
	_handshake.clear();
	_socket.close();

	NOTE("RTMFP server stops");
}

UInt8 RTMFPServer::p2pHandshake(const string& tag,PacketWriter& response,const SocketAddress& address,const UInt8* peerIdWanted) {

	// find the flash client equivalence
	Session* pSession = NULL;
	Sessions::Iterator it;
	for(it=_sessions.begin();it!=_sessions.end();++it) {
		pSession = it->second;
		if(memcmp(pSession->peer().address.addr(),address.addr(),address.length())==0 && pSession->peer().address.port() == address.port())
			break;
	}
	if(it==_sessions.end())
		pSession=NULL;

	Session* pSessionWanted = _sessions.find(peerIdWanted);

	if(_pCirrus) {
		// Just to make working the man in the middle mode !

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
		DEBUG("UDP Hole punching : session wanted not found, must be dead");
		return 0;
	} else if(pSessionWanted->failed()) {
		DEBUG("UDP Hole punching : session wanted is deleting");
		return 0;
	}

	if(_middle) {
		if(pSessionWanted->pTarget) {
			_handshake.createCookie(response,new Cookie(*pSessionWanted->pTarget));
			response.writeRaw("\x81\x02\x1D\x02");
			response.writeRaw(pSessionWanted->pTarget->publicKey,KEY_SIZE);
			return 0x70;
		} else
			ERROR("Peer/peer dumped exchange impossible : no corresponding 'Target' with the session wanted");
	}
	
	/// Udp hole punching normal process
	pSessionWanted->p2pHandshake(address,tag,pSession);

	response.writeAddress(pSessionWanted->peer().address,true);
	vector<Address>::const_iterator it2;
	for(it2=pSessionWanted->peer().privateAddress.begin();it2!=pSessionWanted->peer().privateAddress.end();++it2) {
		const Address& addr = *it2;
		if(addr == address)
			continue;
		response.writeAddress(addr,false);
	}
	
	return 0x71;

}

UInt32 RTMFPServer::createSession(UInt32 farId,const Peer& peer,const UInt8* decryptKey,const UInt8* encryptKey,Cookie& cookie) {
	while(_nextIdSession==0 || _sessions.find(_nextIdSession))
		++_nextIdSession;

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

	Session* pSession;
	if(pTarget) {
		pSession = new Middle(_nextIdSession,farId,peer,decryptKey,encryptKey,_socket,_handler,_sessions,*pTarget);
		DEBUG("500ms sleeping to wait cirrus handshaking");
		Thread::sleep(500); // to wait the cirrus handshake
		pSession->manage();
	} else {
		pSession = new Session(_nextIdSession,farId,peer,decryptKey,encryptKey,_socket,_handler);
		pSession->pTarget = cookie.pTarget;
	}

	_sessions.add(pSession);

	return _nextIdSession;
}

void RTMFPServer::manage() {
	if(!_timeLastManage.isElapsed(_freqManage))
		return;
	_timeLastManage.update();
	_handshake.manage();
	_sessions.manage();
}


} // namespace Cumulus
