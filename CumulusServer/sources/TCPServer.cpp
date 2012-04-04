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

#include "TCPServer.h"
#include "Logs.h"

using namespace std;
using namespace Cumulus;
using namespace Poco;
using namespace Poco::Net;


TCPServer::TCPServer(SocketManager& manager) : _port(0),manager(manager) {
}


TCPServer::~TCPServer() {
	stop();
}

bool TCPServer::start(UInt16 port) {
	if(port==0) {
		ERROR("TCPServer port have to be superior to 0");
		return false;
	}
	try {
		_socket.bind(port);
		_socket.setBlocking(false);
		_socket.listen();
		manager.add(_socket,*this);
		_port=port;
	} catch(Exception& ex) {
		ERROR("TCPServer starting error: %s",ex.displayText().c_str())
		return false;
	}
	return true;
}

void TCPServer::stop() {
	if(_port==0)
		return;
	manager.remove(_socket);
	_socket.close();
	_port=0;
}

void TCPServer::onReadable(const Socket& socket) {
	try {
		StreamSocket ss = _socket.acceptConnection();
		// enabe nodelay per default: OSX really needs that
		ss.setNoDelay(true);
		clientHandler(ss);
	} catch(Exception& ex) {
		WARN("TCPServer socket acceptation: %s",ex.displayText().c_str());
	}
}

void TCPServer::onError(const Socket& socket,const string& error) {
	ERROR("TCPServer socket: %s",error.c_str())
}
