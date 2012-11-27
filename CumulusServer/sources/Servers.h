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

#include "TCPServer.h"
#include "Broadcaster.h"

class Servers : private TCPServer, private ServersHandler, public Broadcaster {
public:
	Servers(Poco::UInt16 port,ServerHandler& handler,Cumulus::SocketManager& manager,const std::string& targets);
	virtual ~Servers();
	
	void manage();
	void start();
	void stop();

	Broadcaster			initiators;
	Broadcaster			targets;

private:
	Poco::UInt32		flush(const std::string& handler);
	Poco::UInt32		flush(Poco::UInt32 handlerRef);

	void				clientHandler(Poco::Net::StreamSocket& socket);
	void				connection(ServerConnection& server);
	bool				disconnection(ServerConnection& server);


	Poco::UInt8								_manageTimes;

	std::set<ServerConnection*>				_targets;
	std::set<ServerConnection*>				_clients;

	ServerHandler&							_handler;
	Poco::UInt16							_port;
};

inline void Servers::clientHandler(Poco::Net::StreamSocket& socket){
	_clients.insert(new ServerConnection(socket,manager,_handler,*this));
}
