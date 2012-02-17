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


SocketManager::SocketManager() : Startable("SocketManager") {
	start();
}


SocketManager::~SocketManager() {
	stop();
}

void SocketManager::add(SocketManaged& socket) {
	ScopedLock<Mutex> lock(_mutex);
	_sockets[socket.socket] = &socket;
}

void SocketManager::remove(SocketManaged& socket) {
	ScopedLock<Mutex> lock(_mutex);
	map<const Socket,SocketManaged*>::iterator it = _sockets.find(socket.socket);
	if(it==_it)
		++_it;
	_sockets.erase(it);
}

bool SocketManager::realTime() {
	if(!running())
		return true;
	ScopedLock<Mutex> lock(_mutex);
	_it=_sockets.begin();
	bool idle=true;
	while(_it != _sockets.end()) {
		SocketManaged& socket = *_it->second;
		++_it;
		if(socket.error) {
			// convert error code in error string
			try {
				error(socket.socket.impl()->socketError());
			} catch(Exception& ex) {
				socket.onError(ex.displayText());
			}
			continue;
		}
		if(socket.writable) {
			socket.onWritable();
			(bool&)socket.writable = false;
		}
		if(socket.readable) {
			UInt32 available = socket.socket.available();
			socket.onReadable(available);
			(bool&)socket.readable = false;
			if(available>0) // ==0 means graceful disconnection
				idle=false;
		}
	}
	return idle;
}

void SocketManager::run(const volatile bool& terminate) {
	Socket::SocketList readables;
	Socket::SocketList writables;
	Socket::SocketList errors;
	
	while (!terminate) {

		readables.clear();
		writables.clear();
		errors.clear();

		{
			ScopedLock<Mutex> lock(_mutex);
			map<const Socket,SocketManaged*>::const_iterator it;
			for(it = _sockets.begin(); it != _sockets.end(); ++it) {
				SocketManaged& socket = *it->second;
				if(socket.error)
					continue;
				if(!socket.readable)
					readables.push_back(it->first);
				if(socket.haveToWrite() && !socket.writable)
					writables.push_back(it->first);
				errors.push_back(it->first);
			}
		}

		try {
			if (Socket::select(readables, writables, errors, _timeout)) {
				Socket::SocketList::iterator it;
				map<const Socket,SocketManaged*>::const_iterator it2;
				for (it = readables.begin(); it != readables.end(); ++it) {
					ScopedLock<Mutex> lock(_mutex);
					it2 = _sockets.find(*it);
					if(it2 != _sockets.end())
						(bool&)it2->second->readable = true;
				}
				for (it = writables.begin(); it != writables.end(); ++it) {
					ScopedLock<Mutex> lock(_mutex);
					it2 = _sockets.find(*it);
					if(it2 != _sockets.end())
						(bool&)it2->second->writable = true;
				}
				for (it = errors.begin(); it != errors.end(); ++it) {
					ScopedLock<Mutex> lock(_mutex);
					it2 = _sockets.find(*it);
					if(it2 != _sockets.end())
						(bool&)it2->second->error = true;
				}
			} else
				Thread::sleep(1);
		} catch(Exception& ex) {
			WARN("SocketManager error, ",ex.displayText().c_str())
		}

	}

}
