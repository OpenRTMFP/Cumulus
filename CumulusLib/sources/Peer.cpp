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
#include "Logs.h"
#include "Util.h"
#include "string.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Peer::Peer(const Poco::Net::SocketAddress& address):_ping(0),address(address) {
}

Peer::~Peer() {
	unsubscribeGroups();
}

void Peer::unsubscribeGroups() {
	list<Group*>::iterator it=_groups.begin();
	while(it!=_groups.end()) {
		(*it)->_peers.remove(*this);
		_groups.erase(it);
		it = _groups.begin();
	}
}

inline void Peer::setPing(Poco::UInt16 ping) {
	UInt16 oldPing = _ping;
	_ping=ping;
	list<Group*>::const_iterator it;
	for(it=_groups.begin();it!=_groups.end();++it)
		(*it)->_peers.update(*this,oldPing);
}

void Peer::setPrivateAddress(const list<Address>& address) {
	((vector<Address>&)privateAddress).resize(address.size());
	list<Address>::const_iterator it;
	int i=0;
	for(it=address.begin();it!=address.end();++it) {
		const Address& addr = *it;
		// Set a port if egals 0!
		if(addr.port==0)
			((UInt16&)addr.port) = this->address.port();
		((vector<Address>&)privateAddress)[i] = addr;
		++i;
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
