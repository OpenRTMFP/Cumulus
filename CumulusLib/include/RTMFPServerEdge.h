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
#include "RTMFPServer.h"
#include "ServerConnection.h"


namespace Cumulus {

class RTMFPServerEdgeParams {
public:
	RTMFPServerEdgeParams() : port(RTMFP_DEFAULT_PORT*10),udpBufferSize(0),threadPriority(Poco::Thread::PRIO_HIGH),serverAddress("127.0.0.1",RTMFP_DEFAULT_PORT+1) {
	}
	Poco::UInt16				port;
	Poco::UInt32				udpBufferSize;
	Poco::Net::SocketAddress	publicAddress;			
	Poco::Net::SocketAddress	serverAddress;
	Poco::Thread::Priority		threadPriority;
};

class RTMFPServerEdge : private RTMFPServer  {
public:
	RTMFPServerEdge(Poco::UInt32 cores=0);
	virtual ~RTMFPServerEdge();

	void start();
	void start(RTMFPServerEdgeParams& params);
	void stop();
	bool running();

private:
	void			handle(bool& terminate);
	void			manage();
	EdgeSession*	findEdgeSession(Poco::UInt32 id);
	Poco::UInt8		p2pHandshake(const std::string& tag,PacketWriter& response,const Poco::Net::SocketAddress& address,const Poco::UInt8* peerIdWanted);
	Session&		createSession(Poco::UInt32 farId,const Peer& peer,const Poco::UInt8* decryptKey,const Poco::UInt8* encryptKey,Cookie& cookie);
	void			destroySession(Session& session);
	void			repeatCookie(Poco::UInt32 farId,Cookie& cookie);
	void			run();

	void			onReadable(Poco::Net::Socket& socket);
	void			onError(const Poco::Net::Socket& socket,const std::string& error);

	ServerConnection					_serverConnection;

	Poco::Timestamp						_timeLastServerReception;
	Poco::Net::DatagramSocket			_serverSocket;
	Poco::Net::SocketAddress			_serverAddress;
	Poco::Net::SocketAddress			_publicAddress;
	Poco::UInt8							_buff[PACKETRECV_SIZE];
};

inline bool RTMFPServerEdge::running() {
	return RTMFPServer::running();
}

inline void RTMFPServerEdge::stop() {
	RTMFPServer::stop();
}


} // namespace Cumulus
