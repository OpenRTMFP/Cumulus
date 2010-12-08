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
#include "Handshake.h"
#include "PacketWriter.h"
#include "Util.h"
#include "Logs.h"
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
}

Session* RTMFPServer::findSession(PacketReader& reader,const SocketAddress& sender) {
	UInt32 id = RTMFP::Unpack(reader);
 
	// Id session can't be egal to 0 (it's reserved to Handshake)
	if(id==0) {
		DEBUG("Handshaking");
		_pHandshake->setPeerAddress(sender);
		return _pHandshake;
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
	if(!cirrusUrl.empty())
		INFO("Mode 'man in the middle' activated : the exchange bypass to '%s'",cirrusUrl.c_str())
	_pHandshake = new Handshake(_sessions,_socket,_database,cirrusUrl);
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

	NOTE("RTMFP server starts on %hu port",_port);

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

		Session* pSession = this->findSession(packet,sender);
		if(!pSession)
			continue;
		
		if(!pSession->decode(packet)) {
			// TODO Log
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


} // namespace Cumulus
