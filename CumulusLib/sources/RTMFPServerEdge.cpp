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

#include "RTMFPServerEdge.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;


namespace Cumulus {

RTMFPServerEdge::RTMFPServerEdge() : RTMFPServer("RTMFPServerEdge"),_serverConnection(*this,_handshake) {

}

RTMFPServerEdge::~RTMFPServerEdge() {

}

EdgeSession* RTMFPServerEdge::findEdgeSession(UInt32 id) {
	EdgeSession* pSession= dynamic_cast<EdgeSession*>(findSession(id));
	if(pSession)
		return pSession;
	DEBUG("Unknown edge session %u",id);
	return NULL;
}

void RTMFPServerEdge::start(RTMFPServerEdgeParams& params) {
	RTMFPServerParams serverParams;
	serverParams.threadPriority = params.threadPriority;

	serverParams.port = params.port;
	if(serverParams.port==0) {
		ERROR("RTMFPServerEdge port must have a positive value");
		return;
	}
	serverParams.udpBufferSize = params.udpBufferSize;
	if(params.serverAddress.host().isWildcard()) {
		ERROR("RTMFPServerEdge server host must be valid (not wildcard)");
		return;
	}
	if(params.serverAddress.port()==0) {
		ERROR("RTMFPServerEdge server port must have a positive value");
		return;
	}
	_serverAddress = params.serverAddress;

	if(params.publicAddress.host().isWildcard())
		_publicAddress = SocketAddress("127.0.0.1",params.publicAddress.port()==0 ? serverParams.port : params.publicAddress.port());
	else if(params.publicAddress.port()==0)
		_publicAddress = SocketAddress(params.publicAddress.host(),serverParams.port);
	else
		_publicAddress = params.publicAddress;

	RTMFPServer::start(serverParams);
	_serverSocket.setReceiveBufferSize(udpBufferSize);_serverSocket.setSendBufferSize(udpBufferSize);
}


void RTMFPServerEdge::run(const volatile bool& terminate) {
	_serverSocket.connect(_serverAddress);
	_serverConnection.setEndPoint(_serverSocket,_serverAddress);

	NOTE("Wait RTMFP server %s successful connection...",_serverAddress.toString().c_str());

	Timestamp connectAttemptTime;	
	_serverConnection.connect(_publicAddress);

	while(!terminate) {

		if(!serverHandler()) {
			if(connectAttemptTime.isElapsed(6000000)) {
				_serverConnection.connect(_publicAddress);
				connectAttemptTime.update();
			}
			Thread::sleep(1);
		}

		if(_serverConnection.connected()) {
			NOTE("RTMFP server %s successful connection",_serverAddress.toString().c_str());
			RTMFPServer::run(terminate);
			_serverConnection.clear();
			if(_serverConnection.connected()) {
				// Exception in RTMFPServer::run OR a volontary stop
				_serverConnection.setEndPoint(_serverSocket,_serverAddress);
				_serverConnection.disconnect();
				break; 
			}
			NOTE("RTMFP server %s connection lost",_serverAddress.toString().c_str());
		}

	}
	_serverSocket.close();
}


Session& RTMFPServerEdge::createSession(UInt32 farId,const Peer& peer,const UInt8* decryptKey,const UInt8* encryptKey,Cookie& cookie) {
	EdgeSession* pSession = new EdgeSession(_sessions.nextId(),farId,peer,decryptKey,encryptKey,_serverSocket,cookie);
	pSession->setEndPoint(_socket,peer.address);
	_serverConnection.createSession(*pSession,cookie.queryUrl);
	_sessions.add(pSession);
	(UInt8&)cookie.response=0;
	return *pSession;
}

void RTMFPServerEdge::destroySession(Session& session) {
	
}

void RTMFPServerEdge::repeatCookie(UInt32 farId,Cookie& cookie) {
	EdgeSession* pSession = findEdgeSession(cookie.id);
	if(pSession)
		_serverConnection.createSession(*pSession,cookie.queryUrl);
}

UInt8 RTMFPServerEdge::p2pHandshake(const string& tag,PacketWriter& response,const SocketAddress& address,const UInt8* peerIdWanted) {
	_serverConnection.sendP2PHandshake(tag,address,peerIdWanted);
	return 0;
}

bool RTMFPServerEdge::realTime(bool& terminate) {
	
	RTMFPServer::realTime(terminate);
	
	if(_serverConnection.died) {
		terminate = true;
		_serverConnection.disconnect();
		return false;
	}
	
	return !serverHandler();
}

bool RTMFPServerEdge::serverHandler() {
	bool result=false;
	try {
		if(_serverSocket.available()>0) {

			int size = _serverSocket.receiveBytes(_buff,sizeof(_buff));
			_timeLastServerReception.update();

			if(size>=RTMFP_MIN_PACKET_SIZE) {
				PacketReader packet(_buff,size);
				UInt32 id = RTMFP::Unpack(packet);
				if(id>0) {
					EdgeSession* pSession= findEdgeSession(id);
					if(pSession)
						pSession->serverPacketHandler(packet);
				} else
					_serverConnection.receive(packet);
			} else
				ERROR("Invalid packet");
			
		}
		result=_serverSocket.available()>0;
	} catch(Exception& ex) {
		WARN("Server socket reception error : %s",ex.displayText().c_str());
	}
	return result;
}

void RTMFPServerEdge::manage() {
	RTMFPServer::manage();
	_serverConnection.manage();
	if(_timeLastServerReception.isElapsed(40000000)) // timeout of 40 sec, server is death!
		(bool&)_serverConnection.died=true;
}

void RTMFPServerEdge::displayCount(UInt32 sessions) {
	INFO("%u clients",sessions);
}


} // namespace Cumulus
