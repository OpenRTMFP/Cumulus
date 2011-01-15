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
#include "string.h"


using namespace std;
using namespace Poco;
using namespace Poco::Net;


namespace Cumulus {

RTMFPServer::RTMFPServer(UInt8 keepAliveServer,UInt8 keepAlivePeer) : _pClientHandler(NULL),_terminate(false),_pCirrus(NULL),_handshake(*this,_socket,_data),_data(keepAliveServer,keepAlivePeer) {
#ifndef _WIN32
//	static const char rnd_seed[] = "string to make the random number generator think it has entropy";
//	RAND_seed(rnd_seed, sizeof(rnd_seed));
#endif
}


RTMFPServer::RTMFPServer(ClientHandler& clientHandler,UInt8 keepAliveServer,UInt8 keepAlivePeer) : _pClientHandler(&clientHandler),_terminate(false),_pCirrus(NULL),_handshake(*this,_socket,_data),_data(keepAliveServer,keepAlivePeer) {
#ifndef _WIN32
//	static const char rnd_seed[] = "string to make the random number generator think it has entropy";
//	RAND_seed(rnd_seed, sizeof(rnd_seed));
#endif
}

RTMFPServer::~RTMFPServer() {
	stop();
}

Session* RTMFPServer::findSession(UInt32 id,const SocketAddress& sender) {
 
	// Id session can't be egal to 0 (it's reserved to Handshake)
	if(id==0) {
		DEBUG("Handshaking");
		((SocketAddress&)_handshake.peer().address) = sender;
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


void RTMFPServer::start(UInt16 port,const SocketAddress* pCirrus) {
	ScopedLock<FastMutex> lock(_mutex);
	if(_mainThread.isRunning()) {
		ERROR("RTMFPServer server is yet running, call stop method before");
		return;
	}
	_port = port;
	_sessions.freqManage(2);
	if(pCirrus) {
		_pCirrus = new Cirrus(*pCirrus,_sessions);
		_sessions.freqManage(0); // no waiting, direct process in the middle case!
	}
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

		UInt32 idSession = RTMFP::Unpack(packet);
		Session* pSession = this->findSession(idSession,sender);

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

	INFO("RTMFP server stopping");
	
	_sessions.clear();
	_handshake.clear();
	_socket.close();

	NOTE("RTMFP server stops");
}


UInt32 RTMFPServer::createSession(UInt32 farId,const Peer& peer,const UInt8* decryptKey,const UInt8* encryptKey) {
	UInt32 id = 0;
	RandomInputStream ris;
	while(id==0 || _sessions.find(id))
		ris.read((char*)(&id),4);

	if(_pCirrus) {
		Middle* pMiddle = new Middle(id,farId,peer,decryptKey,encryptKey,_socket,_data,*_pCirrus);
		_sessions.add(pMiddle);
		DEBUG("500ms sleeping to wait cirrus handshaking");
		Thread::sleep(500); // to wait the cirrus handshake
		pMiddle->manage();
	} else
		_sessions.add(new Session(id,farId,peer,decryptKey,encryptKey,_socket,_data,_pClientHandler));

	return id;
}


UInt8 RTMFPServer::p2pHandshake(const string& tag,PacketWriter& response,const Peer& peer,const UInt8* peerIdWanted) {

	Session* pSessionWanted = _sessions.find(peerIdWanted);
	if(!pSessionWanted) {
		ERROR("UDP Hole punching error : session wanted not found");
		return 0;
	}

	pSessionWanted->p2pHandshake(peer);

	/// Udp hole punching
	if(!_pCirrus) {
		// Normal mode
		response.write8(0x01);
		response.writeAddress(pSessionWanted->peer().address);
		vector<SocketAddress>::const_iterator it;
		for(it=pSessionWanted->peer().privateAddress.begin();it!=pSessionWanted->peer().privateAddress.end();++it) {
			response.write8(0x01);
			response.writeAddress(*it);
		}

		return 0x71;
	} else {
		// Just to make working the man in the middle mode !

		// find the flash client equivalence
		Middle* pMiddle = NULL;
		Sessions::Iterator it;
		for(it=_sessions.begin();it!=_sessions.end();++it) {
			pMiddle = (Middle*)it->second;
			if(memcmp(pMiddle->peer().address.addr(),peer.address.addr(),sizeof(struct sockaddr))==0)
				break;
		}
		if(it==_sessions.end()) {
			ERROR("UDP Hole punching error : middle equivalence not found for session wanted");
			return 0;
		}

		PacketWriter& request = pMiddle->handshaker();
		request.write8(0x22);request.write8(0x21);
		request.write8(0x0F);

		request.writeRaw(((Middle*)pSessionWanted)->middlePeer().id,32);

		pMiddle->pPeerWanted = &pSessionWanted->peer();
		request.writeRaw(tag);

		pMiddle->sendHandshakeToCirrus(0x30);
		// no response here!
	}
	
	return 0;

}


} // namespace Cumulus
