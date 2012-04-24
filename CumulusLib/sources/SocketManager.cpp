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
	SocketManagedImpl(poco_socket_t sockfd,SocketManaged& socketManaged):SocketImpl(sockfd),pSocketManaged(&socketManaged) {}
	virtual ~SocketManagedImpl(){
		reset(); // to avoid the "close" on destruction!
	}
	SocketManaged*		pSocketManaged;
};


class PublicSocket : public Socket {
public:
	PublicSocket(SocketImpl* pImpl):Socket(pImpl) {}
};


class SocketManaged : public Socket {
public:
	SocketManaged(const Socket& socket,SocketHandler& handler):Socket(socket),socketSelectable(new SocketManagedImpl(socket.impl()->sockfd(),*this)),handler(handler) {}
	SocketHandler&				handler;
	PublicSocket				socketSelectable;
};



SocketManager::SocketManager(TaskHandler& handler,const string& name) : Task(handler,name) {
	start();
}


SocketManager::~SocketManager() {
	stop();
	map<const Socket*,SocketManaged*>::iterator it;
	for(it=_sockets.begin();it!= _sockets.end();++it)
		delete it->second;
}

void SocketManager::add(const Socket& socket,SocketHandler& handler) {
	ScopedLock<Mutex> lock(_mutex);
	map<const Socket*,SocketManaged*>::iterator it = _sockets.lower_bound(&socket);
	if(it!=_sockets.end() && it->first==&socket)
		return;
	if(it!=_sockets.begin())
		--it;
	_sockets.insert(it,pair<const Socket*,SocketManaged*>(&socket,new SocketManaged(socket,handler)));
}

void SocketManager::remove(const Socket& socket) {
	ScopedLock<Mutex> lock(_mutex);
	map<const Socket*,SocketManaged*>::iterator it = _sockets.find(&socket);
	if(it == _sockets.end())
		return;
	((SocketManagedImpl*)it->second->socketSelectable.impl())->pSocketManaged = NULL;
	delete it->second;
	_sockets.erase(it);
}

void SocketManager::handle() {
	Socket::SocketList::iterator it;
	SocketManaged*		pSocketManaged;
	for (it = _readables.begin(); it != _readables.end(); ++it) {
		ScopedLock<Mutex> lock(_mutex);
		pSocketManaged = ((SocketManagedImpl*)it->impl())->pSocketManaged;
		if(pSocketManaged) {
			try {
				pSocketManaged->handler.onReadable(*pSocketManaged);
			} catch(Exception& ex) {
				pSocketManaged->handler.onError(*pSocketManaged,ex.displayText().c_str());
			}
		}
	}
	for (it = _writables.begin(); it != _writables.end(); ++it) {
		ScopedLock<Mutex> lock(_mutex);
		pSocketManaged = ((SocketManagedImpl*)it->impl())->pSocketManaged;
		if(pSocketManaged) {
			try {
				pSocketManaged->handler.onWritable(*pSocketManaged);
			} catch(Exception& ex) {
				pSocketManaged->handler.onError(*pSocketManaged,ex.displayText().c_str());
			}
		}
	}	
	for (it = _errors.begin(); it != _errors.end(); ++it) {
		ScopedLock<Mutex> lock(_mutex);
		pSocketManaged = ((SocketManagedImpl*)it->impl())->pSocketManaged;
		if(pSocketManaged) {
			try {
				error(pSocketManaged->impl()->socketError());
			} catch(Exception& ex) {
				pSocketManaged->handler.onError(*pSocketManaged,ex.displayText().c_str());
			}
		}
	}
}


void SocketManager::run() {

	
	Timespan			timeout(50000);
	
	while(running()) {

		bool empty=true;
		{
			ScopedLock<Mutex> lock(_mutex);
			_readables.resize(_sockets.size());
			_errors.resize(_sockets.size());
			_writables.clear();

			int i=0;
			map<const Socket*,SocketManaged*>::iterator it;
			for(it = _sockets.begin(); it != _sockets.end(); ++it) {
				empty=false;
				SocketManaged& socket = *it->second;
				_readables[i] = socket.socketSelectable;
				if(socket.handler.haveToWrite(socket))
					_writables.push_back(socket.socketSelectable);
				_errors[i] = socket.socketSelectable;
				++i;
			}
		}
		if(empty) {
			sleep(timeout.milliseconds());
			continue;
		}

		try {
			if (Socket::select(_readables, _writables, _errors, timeout)==0)
				continue;
		} catch(Exception& ex) {
			WARN("Socket error, %s",ex.displayText().c_str())
		}
		waitHandle();
	}

	
}


} // namespace Cumulus
