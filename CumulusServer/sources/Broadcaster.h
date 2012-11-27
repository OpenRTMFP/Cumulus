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

#include "ServerConnection.h"
#include <set>

class Broadcaster {
	friend class Servers;
public:
	Broadcaster(){}
	virtual ~Broadcaster(){}
	
	typedef std::set<ServerConnection*>::const_iterator Iterator;
	
	Iterator begin() const;
	Iterator end() const;
	ServerConnection* operator[](Poco::UInt32 index);
	ServerConnection* operator[](const std::string& address);
	Poco::UInt32 count() const;
	
	void broadcast(const std::string& handler,ServerMessage& message);

private:
	std::set<ServerConnection*>				_connections;
};

inline Broadcaster::Iterator Broadcaster::begin() const {
	return _connections.begin();
}

inline Poco::UInt32 Broadcaster::count() const {
	return _connections.size();
}

inline Broadcaster::Iterator Broadcaster::end() const {
	return _connections.end();
}

inline ServerConnection* Broadcaster::operator[](const std::string& address) {
	Iterator it;
	for(it=begin();it!=end();++it) {
		if((*it)->address==address || (*it)->publicAddress==address)
			return *it;
	}
	return *it;
}

inline ServerConnection* Broadcaster::operator[](Poco::UInt32 index) {
	if(index>=_connections.size())
		return NULL;
	Iterator it = begin();
	advance(it,index);
	return *it;
}

inline void Broadcaster::broadcast(const std::string& handler,ServerMessage& message) {
	Iterator it;
	for(it=begin();it!=end();++it)
		(*it)->send(handler,message);
}
