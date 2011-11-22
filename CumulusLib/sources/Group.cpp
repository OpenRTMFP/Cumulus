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
#include "string.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

Group::Group(const UInt8* id) {
	memcpy((UInt8*)this->id,id,ID_SIZE);
}

Group::~Group() {

}


void Group::addPeer(Peer& peer) {
	if(!peer.isIn(*this)) {
		peer._groups.push_back(this);
		_peers.insert(&peer);
		if(_lastPeers.size()==6)
			_lastPeers.pop_front();
		_lastPeers.push_back(&peer);
	}
}

void Group::removePeer(Peer& peer) {
	list<Group*>::iterator itG;
	if(peer.isIn(*this,itG)) {
		peer._groups.erase(itG);
		_peers.erase(&peer);
		list<Peer*>::iterator itP;
		for(itP=_lastPeers.begin();itP!=_lastPeers.end();++itP) {
			if((**itP)==peer) {
				_lastPeers.erase(itP);
				break;
			}
		}
	}
}

bool Group::hasPeer(const UInt8* id) {
	set<Peer*>::const_iterator it;
	for(it=_peers.begin();it!=_peers.end();++it) {
		if((**it)==id)
			return true;
	}
	return false;
}


} // namespace Cumulus
