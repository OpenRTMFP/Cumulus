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

#include "Cumulus.h"
#include "Session.h"
#include "Poco/Timestamp.h"

namespace Cumulus {

class Edge {
	friend class Handshake;
	friend class RTMFPServer;
public:
	Address				address;
	
	const Poco::UInt32	count;
private:
	Edge();
	virtual ~Edge();

	bool obsolete();
	bool raise();
	void update();
	void addSession(Session& session);
	void removeSession(Session& session);

	Poco::Timestamp					_timeLastExchange;
	Poco::UInt8						_raise;
	std::map<Poco::UInt32,Session*>	_sessions;
};

inline bool Edge::obsolete() {
	return _timeLastExchange.isElapsed(15000000); // 15 sec
}

inline void Edge::addSession(Session& session) {
	_sessions[session.id] = &session;
}
inline void Edge::removeSession(Session& session) {
	_sessions.erase(session.id);
}

inline bool Edge::raise() {
	return (_raise++)==10;
}

class Edges {
public:
	Edges(std::map<std::string,Edge*>& edges) : _edges(edges) {}
	~Edges(){}

	typedef std::map<std::string,Edge*>::iterator Iterator;

	Iterator		begin();
	Iterator		end();
	Poco::UInt32	count();

	Edge* operator()(const Poco::Net::SocketAddress& address);
private:

	std::map<std::string,Edge*>&	_edges;
};

inline Poco::UInt32 Edges::count() {
	return _edges.size();
}

inline Edges::Iterator Edges::begin() {
	return _edges.begin();
}

inline Edges::Iterator Edges::end() {
	return _edges.end();
}


} // namespace Cumulus
