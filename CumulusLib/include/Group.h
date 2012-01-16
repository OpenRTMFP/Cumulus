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
#include "Peer.h"
#include <map>

namespace Cumulus {


class GroupIterator {
	friend class Group;
public:
	GroupIterator():_pPeers(NULL){}
	bool		  operator !=(const GroupIterator& other) { return _it!=other._it; }
	bool		  operator ==(GroupIterator& other) { return _it==other._it; }
	GroupIterator operator ++(int count) { std::advance(_it,count); return *this; }
    GroupIterator operator ++() { ++_it; return *this; }
    const Client* operator *() { if(_pPeers && _it!=_pPeers->end()) return _it->second; return NULL; }
private:
	GroupIterator(std::map<Poco::UInt32,const Peer*>& peers,bool end=false) : _pPeers(&peers),_it(end ? peers.end() : peers.begin()) { }
	std::map<Poco::UInt32,const Peer*>*  _pPeers; 
	std::map<Poco::UInt32,const Peer*>::const_iterator _it;
};


class Group : public Entity {
	friend class Peer;
public:
	Group(const Poco::UInt8* id);
	virtual ~Group();

	typedef GroupIterator Iterator;

	Iterator begin();
	Iterator end();
	Poco::UInt32  size();

private:
	std::map<Poco::UInt32,const Peer*> 	_peers;
};

inline Group::Iterator Group::begin() {
	return GroupIterator(_peers);
}

inline Group::Iterator Group::end() {
	return GroupIterator(_peers,true);
}

inline Poco::UInt32 Group::size() {
	return _peers.size();
}


} // namespace Cumulus
