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

#include "Group.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

Group::Group(const vector<UInt8>& id) : _id(id) {
}

Group::~Group() {
}



void Group::addPeer(Peer& peer) {
	if(!peer.isIn(*this)) {
		peer._groups.push_back(this);
		_peers.push_back(&peer);
	}
}

void Group::removePeer(Peer& peer) {
	list<Group*>::const_iterator itG;
	if(peer.isIn(*this,itG)) {
		peer._groups.erase(itG);
		list<Peer*>::const_iterator it =_peers.begin();
		for(it =_peers.begin();it != _peers.end();++it) {
			if((**it) == peer) {
				_peers.erase(it);
				break;
			}
		}
	}
}

void Group::clear() {
	list<Peer*>::const_iterator it = _peers.begin();
	while(it!=_peers.end()) {
		removePeer(**it);
		it = _peers.begin();
	}
}


} // namespace Cumulus
