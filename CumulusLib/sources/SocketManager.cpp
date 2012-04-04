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

#include "SocketManager.h"
#include "Logs.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {


class SocketPublic : public Socket {
public:
	SocketPublic(SocketImpl* pImpl):Socket(pImpl){}
};

class SocketManagedImpl : public SocketImpl {
public:
	SocketManagedImpl(const Socket& socket,SocketHandler& handler):socket(socket),SocketImpl(socket.impl()->sockfd()),handler(handler) {}
	virtual ~SocketManagedImpl() {}
	SocketHandler&				handler;
	const Socket&				socket;
};

class SocketManaged : public Socket {
public:
	SocketManaged(const Socket& socket,SocketHandler& handler):Socket(socket),socket(new SocketManagedImpl(socket,handler)),handler(handler) {}
	SocketHandler&				handler;
	SocketPublic				socket;
};



SocketManager::SocketManager() {
}


SocketManager::~SocketManager() {
}

void SocketManager::add(const Socket& socket,SocketHandler& handler) {
	_sockets.insert(SocketManaged(socket,handler));
}

void SocketManager::remove(const Socket& socket) {
	_sockets.erase((SocketManaged&)socket);
}

bool SocketManager::process(const Poco::Timespan& timeout) {
	if(_sockets.empty())
		return false;;

	Socket::SocketList readables(_sockets.size());
	Socket::SocketList writables;
	Socket::SocketList errors(_sockets.size());

	int i=0;
	set<SocketManaged>::iterator it;
	for(it = _sockets.begin(); it != _sockets.end(); ++it) {
		const SocketManaged& socket   = *it;
		readables[i] = socket.socket;
		if(socket.handler.haveToWrite(socket))
			writables.push_back(socket.socket);
		errors[i] = socket.socket;
		++i;
	}

	try {
		if (Socket::select(readables, writables, errors, timeout)==0)
			return false;
		Socket::SocketList::iterator it;
		for (it = readables.begin(); it != readables.end(); ++it)
			((SocketManagedImpl*)it->impl())->handler.onReadable(((SocketManagedImpl*)it->impl())->socket);
		for (it = writables.begin(); it != writables.end(); ++it)
			((SocketManagedImpl*)it->impl())->handler.onWritable(((SocketManagedImpl*)it->impl())->socket);	
		for (it = errors.begin(); it != errors.end(); ++it) {
			try {
				error((*it).impl()->socketError());
			} catch(Exception& ex) {
				((SocketManagedImpl*)it->impl())->handler.onError(((SocketManagedImpl*)it->impl())->socket,ex.displayText().c_str());
			}		
		}
	//} catch(IOException&) {
	} catch(Exception& ex) {
		WARN("Socket error, %s",ex.displayText().c_str())
	}
	return true;
}


} // namespace Cumulus
