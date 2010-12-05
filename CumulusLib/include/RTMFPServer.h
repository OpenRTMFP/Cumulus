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

#pragma once

#include "Cumulus.h"
#include "Session.h"
#include "Handshake.h"
#include "PacketReader.h"
#include "Poco/Runnable.h"
#include "Poco/Mutex.h"
#include "Poco/Thread.h"
#include "Poco/Net/DatagramSocket.h"
#include "Poco/Net/SocketAddress.h"


namespace Cumulus {

	class CUMULUS_API RTMFPServer : public Poco::Runnable {
public:
	RTMFPServer();
	virtual ~RTMFPServer();

	void start(Poco::UInt16 port,const std::string& listenCirrusUrl="");
	void stop();
	bool running();

private:
	Session* findSession(PacketReader& reader);
	void run();

	Handshake*							_pHandshake;
	std::map<Poco::UInt32,Session*>	_sessions;

	volatile bool				_terminate;
	Poco::FastMutex				_mutex;
	Poco::UInt16				_port;
	Poco::Thread				_mainThread;
	Poco::Net::DatagramSocket	_socket;
};

inline bool RTMFPServer::running() {
	return _mainThread.isRunning();
}


} // namespace Cumulus
