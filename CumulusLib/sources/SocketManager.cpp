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



class SocketManagedImpl : public SocketImpl {
public:
	SocketManagedImpl(const Socket& socket,SocketHandler& handler):SocketImpl(socket.impl()->sockfd()),pHandler(&handler) {}
	SocketHandler*		pHandler;
private:
	void close() {
	}
};

class SocketManaged : public Socket {
public:
	SocketManaged(const Socket& socket,SocketHandler& handler):Socket(socket),socket(new SocketManagedImpl(socket,handler)),handler(handler) {}
	SocketHandler&				handler;
	Socket						socket;
};



SocketManager::SocketManager() {
}


SocketManager::~SocketManager() {
}

void SocketManager::add(const Socket& socket,SocketHandler& handler) {
	_sockets.insert(SocketManaged(socket,handler));
}

void SocketManager::remove(const Socket& socket) {
	set<SocketManaged>::iterator it = _sockets.find((SocketManaged&)socket);
	if(it == _sockets.end())
		return;
	((SocketManagedImpl*)it->socket.impl())->pHandler = NULL;
	_sockets.erase(it);
}

bool SocketManager::process(const Poco::Timespan& timeout) {
	if(_sockets.empty())
		return false;;

	if(_readables.size()!=_sockets.size())
		_readables.resize(_sockets.size());
	if(_errors.size()!=_sockets.size())
		_errors.resize(_sockets.size());
	_writables.clear();

	int i=0;
	set<SocketManaged>::iterator it;
	for(it = _sockets.begin(); it != _sockets.end(); ++it) {
		const SocketManaged& socket   = *it;
		_readables[i] = socket.socket;
		if(socket.handler.haveToWrite(socket))
			_writables.push_back(socket.socket);
		_errors[i] = socket.socket;
		++i;
	}

	try {
		if (Socket::select(_readables, _writables, _errors, timeout)==0)
			return false;
		Socket::SocketList::iterator it;
		for (it = _readables.begin(); it != _readables.end(); ++it) {
			SocketHandler* pHandler = ((SocketManagedImpl*)it->impl())->pHandler;
			if(pHandler)
				pHandler->onReadable(*it);
		}
		for (it = _writables.begin(); it != _writables.end(); ++it) {
			SocketHandler* pHandler = ((SocketManagedImpl*)it->impl())->pHandler;
			if(pHandler)
				pHandler->onWritable(*it);
		}	
		for (it = _errors.begin(); it != _errors.end(); ++it) {
			SocketHandler* pHandler = ((SocketManagedImpl*)it->impl())->pHandler;
			if(!pHandler)
				continue;
			try {
				error((*it).impl()->socketError());
			} catch(Exception& ex) {
				pHandler->onError(*it,ex.displayText().c_str());
			}		
		}
	} catch(Exception& ex) {
		WARN("Socket error, %s",ex.displayText().c_str())
	}
	return true;
}


} // namespace Cumulus
