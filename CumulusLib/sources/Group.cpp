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
		UInt32 index = _peers.size()==0 ? 0 : _peers.rbegin()->first;
		_peers[index] = &peer;
		peer._groups[this] = index;
	}
}

void Group::removePeer(Peer& peer) {
	map<Group*,UInt32>::iterator itG;
	if(peer.isIn(*this,itG)) {
		_peers.erase(itG->second);
		peer._groups.erase(itG);
	}
}

} // namespace Cumulus
