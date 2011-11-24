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

#include "Peer.h"
#include "Group.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

Peer::Peer():ping(0),connected(false) {
}

Peer::~Peer() {
	unsubscribeGroups();
}

void Peer::unsubscribeGroups() {
	map<Group*,UInt32>::const_iterator it=_groups.begin();
	while(it!=_groups.end()) {
		it->first->removePeer(*this);
		it = _groups.begin();
	}
	if(_groups.size()!=0)
		CRITIC("unsubscribeGroups fails, it stay always some group subscription for %s peer",address.toString().c_str())
}

bool Peer::isIn(Group& group) {
	map<Group*,UInt32>::iterator it;
	return isIn(group,it);
}

bool Peer::isIn(Group& group,map<Group*,UInt32>::iterator& it) {
	it = _groups.find(&group);
	return it != _groups.end();
}


} // namespace Cumulus
