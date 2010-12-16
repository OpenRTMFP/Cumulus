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
#include "PacketWriter.h"
#include "Util.h"
#include "Logs.h"
#include "Poco/RandomStream.h"


using namespace std;
using namespace Poco;
using namespace Poco::Net;


namespace Cumulus {

RTMFPServer::RTMFPServer(UInt16 keepAlivePeer,UInt16 keepAliveServer) : _terminate(false),_pCirrus(NULL),_sessions(2),_handshake(*this,_socket,_data),_data(keepAlivePeer,keepAliveServer) {
#ifndef _WIN32
//	static const char rnd_seed[] = "string to make the random number generator think it has entropy";
//	RAND_seed(rnd_seed, sizeof(rnd_seed));
#endif
}

RTMFPServer::~RTMFPServer() {
	stop();
}

Session* RTMFPServer::findSession(PacketReader& reader,const SocketAddress& sender) {

	UInt32 id = RTMFP::Unpack(reader);
 
	// Id session can't be egal to 0 (it's reserved to Handshake)
	if(id==0) {
		DEBUG("Handshaking");
		_handshake.setPeerAddress(sender);
		return &_handshake;
	}
	Session* pSession = _sessions.find(id);
	if(pSession) {
		DEBUG("Session d'identification '%u'",id);
		return pSession;
	}
	WARN("Unknown session '%u'",id);
	return NULL;
}


void RTMFPServer::start(UInt16 port,const string& cirrusUrl) {
	ScopedLock<FastMutex> lock(_mutex);
	if(_mainThread.isRunning()) {
		ERROR("RTMFPServer server is yet running, call stop method before");
		return;
	}
	_port = port;
	if(!cirrusUrl.empty()) {
		INFO("Mode 'man in the middle' activated : the exchange bypass to '%s'",cirrusUrl.c_str())
		_pCirrus = new Cirrus(cirrusUrl,_sessions);
	}
	_terminate = false;
	_mainThread.start(*this);
	
}

void RTMFPServer::stop() {
	ScopedLock<FastMutex> lock(_mutex);
	_terminate = true;
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
	UInt8 buff[MAX_SIZE_MSG];
	int size = 0;
	Timespan span(250000);

	NOTE("RTMFP server starts on %hu port",_port);

	while(!_terminate) {

		_sessions.manage();

		try {
			if (!_socket.poll(span, Socket::SELECT_READ))
				continue;
			size = _socket.receiveFrom(buff,MAX_SIZE_MSG,sender);
		} catch(Exception& ex) {
			ERROR("Main socket reception error : %s",ex.displayText().c_str());
			continue;
		}

		PacketReader packet(buff,size);
		if(!RTMFP::IsValidPacket(packet)) {
			ERROR("Invalid packet");
			continue;
		}

		Session* pSession = this->findSession(packet,sender);

		if(!pSession)
			continue;

		if(!pSession->decode(packet)) {
			ERROR("Decrypt error");
			continue;
		}
	
		if(Logs::Dump()) {
			printf("Request:\n");
			Util::Dump(packet,Logs::DumpFile());
		}

		pSession->packetHandler(packet);

	}

	NOTE("RTMFP server stops");

}


UInt32 RTMFPServer::createSession(UInt32 farId,const UInt8* peerId,const SocketAddress& peerAddress,const string& url,const UInt8* decryptKey,const UInt8* encryptKey) {
	UInt32 id = 0;
	RandomInputStream ris;
	while(id==0 || _sessions.find(id))
		ris.read((char*)(&id),4);

	if(_pCirrus) {
		_sessions.add(new Middle(id,farId,peerId,peerAddress,url,decryptKey,encryptKey,_socket,_data,*_pCirrus));
		Sleep(500); // to wait the cirrus handshake 
	} else
		_sessions.add(new Session(id,farId,peerId,peerAddress,url,decryptKey,encryptKey,_socket,_data));

	return id;
}


Poco::UInt8 RTMFPServer::p2pHandshake(const string& tag,PacketWriter& response,const BLOB& peerWantedId,const Poco::Net::SocketAddress& peerAddress) {

	Session* pSessionConnected = _sessions.find(peerWantedId);
	if(!pSessionConnected) {
		CRITIC("UDP Hole punching error!");
		return 0;
	}

	pSessionConnected->p2pHandshake(peerAddress);

	/// Udp hole punching
	if(!_pCirrus) {
		// Normal mode
		vector<string> routes;
		_data.getRoutes(pSessionConnected->peerId(),routes);
	
		for(int i=0;i<routes.size();++i) {
			response.write8(0x01);
			response.writeAddress(SocketAddress(routes[i]));
		}

		return 0x71;
	} else {
		// Just to make working the man in the middle mode !
		
		PacketWriter request(12);
		request.write8(0x22);request.write8(0x21);
		request.write8(0x0F);

		// find the flash client equivalence
		Middle* pMiddle = NULL;
		Sessions::Iterator it;
		for(it=_sessions.begin();it!=_sessions.end();++it) {
			pMiddle = (Middle*)it->second;
			if(memcmp(pMiddle->peerAddress().addr(),peerAddress.addr(),sizeof(struct sockaddr))==0)
				break;
		}
		if(it==_sessions.end()) {
			CRITIC("UDP Hole punching error!");
			return 0;
		}

		request.writeRaw(((Middle*)pSessionConnected)->middlePeerId().begin(),32);

		pMiddle->pPeerAddressWanted = &pSessionConnected->peerAddress();
		request.writeRaw(tag);
		
		pMiddle->sendHandshakeToCirrus(0x30,request);
		// no response here!
	}
	
	return 0;

}


} // namespace Cumulus
