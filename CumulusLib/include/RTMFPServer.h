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

#pragma once

#include "Cumulus.h"
#include "Session.h"
#include "PacketReader.h"
#include "Handshake.h"
#include "ServerData.h"
#include "Cirrus.h"
#include "Gateway.h"
#include "Poco/Runnable.h"
#include "Poco/Mutex.h"
#include "Poco/Thread.h"
#include "Poco/Net/DatagramSocket.h"
#include "Poco/Net/SocketAddress.h"

#define CUMULUS_DEFAULT_PORT 1935

namespace Cumulus {

class CUMULUS_API RTMFPServer : public Poco::Runnable,private Gateway {
public:
	RTMFPServer(Poco::UInt16 keepAliveServer=15000,Poco::UInt16 keepAlivePeer=10000);
	virtual ~RTMFPServer();

	void start();
	void start(Poco::UInt16 port,const std::string& cirrusUrl="");
	void stop();
	bool running();

private:
	Session* findSession(PacketReader& reader,const Poco::Net::SocketAddress& sender);
	void	 run();
	Poco::UInt8		p2pHandshake(const std::string& tag,PacketWriter& response,const Peer& peer,const Poco::UInt8* peerIdWanted);
	Poco::UInt32	createSession(Poco::UInt32 farId,const Peer& peer,const std::string& url,const Poco::UInt8* decryptKey,const Poco::UInt8* encryptKey);

	Handshake					_handshake;

	volatile bool				_terminate;
	Poco::FastMutex				_mutex;
	Poco::UInt16				_port;
	Poco::Thread				_mainThread;
	Poco::Net::DatagramSocket	_socket;

	Sessions					_sessions;
	ServerData					_data;
	Cirrus*						_pCirrus;
};

inline void RTMFPServer::start() {
	start(CUMULUS_DEFAULT_PORT);
}

inline bool RTMFPServer::running() {
	return _mainThread.isRunning();
}


} // namespace Cumulus
