/* 
	Copyright 2010 cumulus.dev@gmail.com
 
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
#include "PacketWriter.h"
#include "Util.h"
#include "Logs.h"
#include "Poco/File.h"
#include "Poco/RandomStream.h"


using namespace std;
using namespace Poco;
using namespace Poco::Net;



namespace Cumulus {

RTMFPServer::RTMFPServer() : _terminate(false),_pHandshake(NULL) {
#ifndef _WIN32
//	static const char rnd_seed[] = "string to make the random number generator think it has entropy";
//	RAND_seed(rnd_seed, sizeof(rnd_seed));
#endif
}

RTMFPServer::~RTMFPServer() {
	stop();
	// delete sessions
	map<UInt32,Session*>::const_iterator it;
	for(it=_sessions.begin();it!=_sessions.end();++it)
		delete it->second;
	_sessions.clear();
}

//assume packet is bigger than 12 bytes since it is static buf anyways...
Session* RTMFPServer::findSession(PacketReader& reader) {
	UInt32 id = RTMFP::Unpack(reader);
 
	// Id session can't be egal to 0 (it's reserved to Handshake)
	if(id==0) {
		DEBUG("Handshaking");
		RandomInputStream ris;
		while(_pHandshake->nextSessionId==0 || _sessions.find(_pHandshake->nextSessionId)!=_sessions.end())
			ris.read((char*)(&_pHandshake->nextSessionId),4);
		
		return _pHandshake;
	}
	if(_sessions.find(id)==_sessions.end()) {
		WARN("Unknown session '%u'",id);
		return NULL;
	}

	DEBUG("Session d'identification '%u'",id);
	return _sessions[id];
}


void RTMFPServer::start(UInt16 port,const string& listenCirrusUrl) {
	ScopedLock<FastMutex> lock(_mutex);
	if(_mainThread.isRunning()) {
		ERROR("RTMFPServer server is yet running, call stop method before");
		return;
	}
	_port = port;
	if(!listenCirrusUrl.empty()) {
		File dumpFile("dump.txt");
		if(dumpFile.exists())
			dumpFile.remove();
		INFO("Mode 'man in the middle' : cirrus/flash exchange are dumped in 'dump.txt'")
	}
	_pHandshake = new Handshake(_socket,listenCirrusUrl);
	_terminate = false;
	_mainThread.start(*this);
	
}

void RTMFPServer::stop() {
	ScopedLock<FastMutex> lock(_mutex);
	_terminate = true;
	_mainThread.join();
	if(_pHandshake) {
		delete _pHandshake;
		_pHandshake = NULL;
	}
}

void RTMFPServer::run() {
	SetThreadName("RTMFPServer");
	SocketAddress address("localhost",_port);
	_socket.bind(address,true);
	
	SocketAddress sender;
	UInt8 buff[MAX_SIZE_MSG];
	int size = 0;
	Timespan span(250000);

	NOTE("RTMFP server starts");

	while(!_terminate) {
		try {
			if (_socket.poll(span, Socket::SELECT_READ))
				size = _socket.receiveFrom(buff,MAX_SIZE_MSG,sender);
			else
				continue;
		} catch(Exception& ex) {
			ERROR("Main socket reception error : %s",ex.displayText().c_str());
			continue;
		}

		PacketReader packet(buff,size);
		if(!RTMFP::IsValidPacket(packet)) {
			// TODO log
			continue;
		}

		Session* pSession = this->findSession(packet);
		if(!pSession)
			continue;
		
		if(!pSession->decode(packet)) {
			// TODO Log
			continue;
		}

		printf("Request:\n");
		Util::Dump(packet);

		pSession->packetHandler(packet,sender);


		// New Session?
		Session* pNewSession = _pHandshake->getNewSession();
		if(pNewSession) {
			_sessions[pNewSession->id()] = pNewSession;
			pSession = pNewSession;
		}

	}

	NOTE("RTMFP server stops");

}


} // namespace Cumulus
