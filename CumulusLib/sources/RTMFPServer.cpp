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
#include "string.h"


using namespace std;
using namespace Poco;
using namespace Poco::Net;


namespace Cumulus {

RTMFPServer::RTMFPServer() : Startable("RTMFPServer"),_pCirrus(NULL),_handshake(*this,_socket,*this),_nextIdSession(0) {
#ifndef _WIN32
//	static const char rnd_seed[] = "string to make the random number generator think it has entropy";
//	RAND_seed(rnd_seed, sizeof(rnd_seed));
#endif
}

RTMFPServer::~RTMFPServer() {

}

Session* RTMFPServer::findSession(UInt32 id) {
 
	// Id session can't be egal to 0 (it's reserved to Handshake)
	if(id==0) {
		DEBUG("Handshaking");
		return &_handshake;
	}
	Session* pSession = _sessions.find(id);
	if(pSession) {
	//	TRACE("Session d'identification '%u'",id);
		if(!pSession->checked)
			_handshake.commitCookie(*pSession);
		return pSession;
	}

	WARN("Unknown session '%u'",id);
	return NULL;
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
	_port = params.port;
	_freqManage = 2000000; // 2 sec by default
	if(params.pCirrus) {
		_pCirrus = new Target(*params.pCirrus);
		_freqManage = 0; // no waiting, direct process in the middle case!
	}
	_middle = params.middle;

	(UInt32&)keepAliveServer = params.keepAliveServer<5 ? 5000 : params.keepAliveServer*1000;
	(UInt32&)keepAlivePeer = params.keepAlivePeer<5 ? 5000 : params.keepAlivePeer*1000;
	
	onStart();
	setPriority(params.threadPriority);
	Startable::start();
}


void RTMFPServer::run(const volatile bool& terminate) {
	SocketAddress address("0.0.0.0",_port);
	_socket.bind(address,true);

	SocketAddress sender;
	UInt8 buff[PACKETRECV_SIZE];
	int size = 0;

	NOTE("RTMFP server starts on %hu port",_port);

	try {

		while(!terminate) {
			manage();

			Thread::sleep(1);

			while(!terminate && _socket.available()>0) {
				manage();

				try {
					size = _socket.receiveFrom(buff,sizeof(buff),sender);
				} catch(Exception& ex) {
					WARN("Main socket reception : %s",ex.message().c_str());
					_socket.close();
					_socket.bind(address,true);
					continue;
				}

				//TRACE("Sender : %s",sender.toString().c_str());

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
			
				Logs::Dump(packet,format("Request from %s",sender.toString()).c_str());

				pSession->packetHandler(packet);
			}

		}
	} catch(Exception& ex) {
		FATAL("RTMFPServer error : %s",ex.message().c_str());
	} catch (exception& ex) {
		FATAL("RTMFPServer error : %s",ex.what());
	} catch (...) {
		FATAL("RTMFPServer unknown error");
	}

	INFO("RTMFP server stopping");
	
	_sessions.clear();
	_handshake.clear();
	_socket.close();

	NOTE("RTMFP server stops");
	onStop();
	if(_pCirrus) {
		delete _pCirrus;
		_pCirrus = NULL;
	}
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
		pSession = new Middle(_nextIdSession,farId,peer,decryptKey,encryptKey,_socket,*this,_sessions,*pTarget);
		if(_pCirrus==pTarget)
			pSession->pTarget = cookie.pTarget;
		DEBUG("500ms sleeping to wait cirrus handshaking");
		Thread::sleep(500); // to wait the cirrus handshake
		pSession->manage();
	} else {
		pSession = new Session(_nextIdSession,farId,peer,decryptKey,encryptKey,_socket,*this);
		pSession->pTarget = cookie.pTarget;
	}

	_sessions.add(pSession);

	return _nextIdSession;
}

void RTMFPServer::manage() {
	if(!_timeLastManage.isElapsed(_freqManage)) {
		// Just middle session!
		if(_middle) {
			Sessions::Iterator it;
			for(it=_sessions.begin();it!=_sessions.end();++it) {
				Middle* pMiddle = dynamic_cast<Middle*>(it->second);
				if(pMiddle)
					pMiddle->manage();
			}
		}
		return;
	}
	_timeLastManage.update();
	_handshake.manage();
	_sessions.manage();
	if(!_middle && !_pCirrus && _timeLastManage.isElapsed(20000))
		WARN("Process management has lasted more than 20ms : %ums",UInt32(_timeLastManage.elapsed()/1000));
}


} // namespace Cumulus
