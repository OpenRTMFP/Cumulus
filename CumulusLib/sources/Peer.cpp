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

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Peer::Peer():_ping(0) {
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

void Peer::setPing(UInt16 ping) {
	// Don't considerate a localhost peer
	if(address.host().isLoopback()) {
		DEBUG("Peer with a loopback address skipped for a bestpeer record");
		return;
	}
	list<Group*>::const_iterator it;
	_ping = ping;
	for(it=_groups.begin();it!=_groups.end();++it) {
		Group* pGroup = *it;
		if(pGroup->_pBestPeer!=this && (!pGroup->_pBestPeer || ping<pGroup->_pBestPeer->getPing()))
			pGroup->_pBestPeer = this;
	}
}

void Peer::addPrivateAddress(const SocketAddress& address) {
	if(memcmp(this->address.addr(),address.addr(),sizeof(struct sockaddr))==0)
		return;
	vector<SocketAddress>::const_iterator it;
	for(it=privateAddress.begin();it!=privateAddress.end();++it) {
		if(memcmp(it->addr(),address.addr(),sizeof(struct sockaddr))==0)
			return;
	}
	((vector<SocketAddress>&)privateAddress).push_back(address);
}

bool Peer::isIn(Group& group) {
	list<Group*>::iterator it;
	return isIn(group,it);
}

bool Peer::isIn(Group& group,list<Group*>::iterator& it) {
	for(it=_groups.begin();it!=_groups.end();++it) {
		if(group == **it)
			return true;
	}
	return false;
}



} // namespace Cumulus
