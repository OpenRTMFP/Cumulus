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
#include "Handshake.h"
#include "Gateway.h"
#include "Sessions.h"
#include "Startable.h"
#include "Handler.h"
#include "Poco/Net/DatagramSocket.h"
#include "Poco/Net/SocketAddress.h"


namespace Cumulus {

class RTMFPServerParams {
public:
	RTMFPServerParams() : port(RTMFP_DEFAULT_PORT),edgesAttemptsBeforeFallback(2),udpBufferSize(0),edgesPort(0),threadPriority(Poco::Thread::PRIO_HIGH),pCirrus(NULL),middle(false),keepAlivePeer(10),keepAliveServer(15) {
	}
	Poco::UInt16				port;
	Poco::UInt32				udpBufferSize;
	Poco::UInt16				edgesPort;
	Poco::UInt8					edgesAttemptsBeforeFallback;
	bool						middle;
	Poco::Net::SocketAddress*	pCirrus;
	Poco::Thread::Priority		threadPriority;

	Poco::UInt16				keepAlivePeer;
	Poco::UInt16				keepAliveServer;
};

class MainSockets : public SocketManager,private TaskHandler {
public:
	MainSockets():SocketManager((TaskHandler&)*this,"MainSockets") {}
	virtual ~MainSockets(){}
private:
	void requestHandle(){giveHandle();}
};


class RTMFPServer : private Gateway,protected Handler,private Startable,private SocketHandler {
	friend class RTMFPServerEdge;
	friend class RTMFPManager;
	friend class RTMFPReceiving;
public:
	RTMFPServer(Poco::UInt32 cores=0);
	virtual ~RTMFPServer();

	void start();
	void start(RTMFPServerParams& params);
	void stop();
	bool running();

protected:
	virtual void    manage();

private:
	RTMFPServer(const std::string& name,Poco::UInt32 cores);
	virtual void    onStart(){}
	virtual void    onStop(){}
	void			requestHandle();
	virtual void	handle(bool& terminate);

	void			receive(RTMFPReceiving& rtmfpReceiving);
	void			prerun();
	void			run();
	Poco::UInt8		p2pHandshake(const std::string& tag,PacketWriter& response,const Poco::Net::SocketAddress& address,const Poco::UInt8* peerIdWanted);
	Session&		createSession(Poco::UInt32 farId,const Peer& peer,const Poco::UInt8* decryptKey,const Poco::UInt8* encryptKey,Cookie& cookie);
	void			destroySession(Session& session);

	void			onReadable(Poco::Net::Socket& socket);
	void			onError(const Poco::Net::Socket& socket,const std::string& error);

	Handshake					_handshake;
	SendingEngine				_sendingEngine;
	ReceivingEngine				_receivingEngine;

	Poco::UInt16				_port;
	Poco::Net::DatagramSocket	_socket;

	Poco::UInt16					_edgesPort;
	Poco::Net::DatagramSocket		_edgesSocket;

	bool							_middle;
	Target*							_pCirrus;
	Sessions						_sessions;
	MainSockets						_mainSockets;
};

inline void	RTMFPServer::requestHandle() {
	wakeUp();
}

inline bool RTMFPServer::running() {
	return Startable::running();
}

inline void RTMFPServer::stop() {
	Startable::stop();
}


} // namespace Cumulus
