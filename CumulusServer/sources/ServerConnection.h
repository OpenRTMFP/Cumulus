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

#include "TCPClient.h"
#include "PacketReader.h"
#include "ServerMessage.h"


class ServerConnection;
class ServerHandler {
public:
	virtual const std::string&	publicAddress()=0;
	virtual Poco::UInt16 port()=0;
	virtual void connection(ServerConnection& server)=0;
	virtual void message(ServerConnection& server,const std::string& handler,Cumulus::PacketReader& reader)=0;
	virtual void disconnection(const ServerConnection& server,const char* error)=0;
};

class ServersHandler {
public:
	virtual void connection(ServerConnection& server)=0;
	virtual void disconnection(ServerConnection& server)=0;
};


class ServerConnection : private TCPClient  {
public:
	ServerConnection(const std::string& address,Cumulus::SocketManager& socketManager,ServerHandler& handler,ServersHandler& serversHandler);
	ServerConnection(const Poco::Net::StreamSocket& socket,Cumulus::SocketManager& socketManager,ServerHandler& handler,ServersHandler& serversHandler);
	virtual ~ServerConnection();

	const std::string address;
	const std::string publicAddress;

	void			connect();

	void			send(const std::string& handler,ServerMessage& message);
private:
	void			sendPublicAddress();

	Poco::UInt32	onReception(const Poco::UInt8* data,Poco::UInt32 size);
	void			onDisconnection();

	ServerHandler&						_handler;
	ServersHandler&						_serversHandler;
	std::map<std::string,Poco::UInt32>	_sendingRefs;
	std::map<Poco::UInt32,std::string>	_receivingRefs;

	Poco::UInt32						_size;
	bool								_connected;
};
