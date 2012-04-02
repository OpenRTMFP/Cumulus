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


#include "SocketManager.h"
#include "Poco/Net/ServerSocket.h"


class TCPServer : private SocketManaged {
public:
	TCPServer(SocketManager& manager);
	virtual ~TCPServer();

	bool			start(Poco::UInt16 port);
	bool			running();
	Poco::UInt16	port();
	void			stop();

protected:
	SocketManager&	manager;

private:
	virtual void	clientHandler(Poco::Net::StreamSocket& socket)=0;


	void	onReadable(Poco::UInt32 available);
	void	onWritable();
	void	onError(const std::string& error);
	bool	haveToWrite();

	Poco::Net::ServerSocket		_socket;
	Poco::UInt16				_port;
};

inline bool TCPServer::haveToWrite() {
	return false;
}

inline bool	TCPServer::running() {
	return _port>0;
}

inline Poco::UInt16	TCPServer::port() {
	return _port;
}
