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

namespace Cumulus {

Peer::Peer():ping(0),connected(false) {
}

Peer::~Peer() {
	unsubscribeGroups();
}

void Peer::unsubscribeGroups() {
	list<Group*>::const_iterator it=_groups.begin();
	while(it!=_groups.end()) {
		(*it)->removePeer(*this);
		it = _groups.begin();
	}
}

bool Peer::isIn(Group& group) const {
	list<Group*>::iterator it;
	return ((Peer&)*this).isIn(group,it);
}

bool Peer::isIn(Group& group,list<Group*>::iterator& it) {
	for(it=_groups.begin();it!=_groups.end();++it) {
		if(group == **it)
			return true;
	}
	return false;
}


} // namespace Cumulus
