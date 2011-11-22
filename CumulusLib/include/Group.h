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
#include <set>

namespace Cumulus {

class Group : public Entity {
	friend class Peer;
	friend class Middle;
public:
	Group(const Poco::UInt8* id);
	virtual ~Group();

	void						addPeer(Peer& peer);
	void						removePeer(Peer& peer);

	const std::list<Peer*>&		lastPeers();
	bool						empty();

private:
	bool						hasPeer(const Poco::UInt8* id);

	std::set<Peer*> 			_peers;
	std::list<Peer*> 			_lastPeers;
};

inline const std::list<Peer*>& Group::lastPeers() {
	return _lastPeers;
}

inline bool Group::empty() {
	return _peers.empty();
}


} // namespace Cumulus
