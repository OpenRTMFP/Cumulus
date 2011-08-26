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

namespace Cumulus {


class Peers {
public:
	Peers();
	virtual ~Peers();
	
	bool has(const Poco::UInt8* peerId);
	void add(const Peer& peer);
	void remove(const Peer& peer);
	void update(const Peer& peer,Poco::UInt16 oldPing);
	void best(std::list<const Peer*>& peers,const Peer& askerPeer) const;
	bool empty() const;
private:
	
	std::list<const Peer*>							_localPeers;
	std::map<Poco::UInt16,std::list<const Peer*>*>	_peers;
};

inline bool Peers::empty() const {
	return _localPeers.size()==0 && _peers.size()==0;
}


} // namespace Cumulus
