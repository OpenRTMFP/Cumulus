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

#include "Peers.h"
#include "Logs.h"

#define		MAX_BEST_PEERS	6

using namespace std;
using namespace Poco;

namespace Cumulus {

Peers::Peers() {
	
}

Peers::~Peers() {
	// delete list
	map<UInt16,list<const Peer*>*>::const_iterator it;
	for(it=_peers.begin();it!=_peers.end();++it)
		delete it->second;
	_peers.clear();
}

void Peers::add(const Peer& peer) {
	list<const Peer*>::const_iterator it;

	if(peer.address().host().isLoopback()) {
		for(it=_localPeers.begin();it!=_localPeers.end();++it) {
			if(*it==&peer)
				return; // already included
		}
		_localPeers.push_front(&peer);
	} else {
		list<const Peer*>& peers(*_peers.insert(pair<UInt16,list<const Peer*>*>(peer.getPing(),new std::list<const Peer*>())).first->second);
		for(it=peers.begin();it!=peers.end();++it) {
			if(*it==&peer)
				return; // already included
		}
		peers.push_back(&peer);
	}
}

void Peers::remove(const Peer& peer) {
	
	if(peer.address().host().isLoopback()) {
		list<const Peer*>::iterator it;
		for(it=_localPeers.begin();it!=_localPeers.end();++it) {
			if(*it==&peer) {
				_localPeers.erase(it);
				break;
			}
		}
	} else {
		map<UInt16,list<const Peer*>*>::const_iterator it = _peers.find(peer.getPing());
		if(it == _peers.end())
			return;
		list<const Peer*>& peers(*it->second);
		list<const Peer*>::iterator itP;
		for(itP=peers.begin();itP!=peers.end();++itP) {
			if(*itP==&peer) {
				peers.erase(itP);
				break;
			}
		}
	}
}

void Peers::update(const Peer& peer,UInt16 oldPing) {
	if(peer.address().host().isLoopback())
		return;
	
	map<UInt16,list<const Peer*>*>::const_iterator it = _peers.find(oldPing);
	if(it == _peers.end()) {
		ERROR("Peer update impossible because the peer is not in the group");
		return;
	}

	list<const Peer*>& peers(*it->second);
	list<const Peer*>::iterator itP;
	for(itP=peers.begin();itP!=peers.end();++itP) {
		if(*itP==&peer) {
			peers.erase(itP);
			_peers.insert(pair<UInt16,list<const Peer*>*>(peer.getPing(),new std::list<const Peer*>())).first->second->push_back(&peer);
			return;
		}
	}
}

void Peers::best(list<const Peer*>& peers,const Peer& askerPeer) const {
	list<const Peer*>::const_iterator itP;
	map<UInt16,list<const Peer*>*>::const_iterator it;
	for(it=_peers.begin();it!=_peers.end();++it) {
		const list<const Peer*>& curPeers(*it->second);
		for(itP=curPeers.begin();itP!=curPeers.end();++itP) {
			if(*itP==&askerPeer)
				continue;
			peers.push_back(*itP);
			if(peers.size()==MAX_BEST_PEERS)
				break;
		}
		if(peers.size()==MAX_BEST_PEERS)
			break;
	}
	if(peers.size()<MAX_BEST_PEERS) {
		for(itP=_localPeers.begin();itP!=_localPeers.end();++itP) {
			if(*itP==&askerPeer)
				continue;
			peers.push_back(*itP);
			if(peers.size()==MAX_BEST_PEERS)
				break;
		}
	}
}



} // namespace Cumulus
