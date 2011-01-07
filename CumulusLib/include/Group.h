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
#include <vector>
#include <list>


namespace Cumulus {

class Group {

public:
	Group(const std::vector<Poco::UInt8>& id);
	virtual ~Group();

	bool operator==(const Group& other) const;
	bool operator==(const std::vector<Poco::UInt8>& id) const;
	bool operator!=(const Group& other) const;
	bool operator!=(const std::vector<Poco::UInt8>& id) const;

	void addPeer(Peer& peer);
	void removePeer(Peer& peer);
	void clear();
	Peer* onePeer();

private:
	std::vector<Poco::UInt8>	_id;
	std::list<Peer*>		_peers;
};


inline Peer* Group::onePeer() {
	return _peers.size()==0 ? NULL : _peers.back();
}
inline bool Group::operator==(const Group& other) const {
	return other==_id;
}

inline bool Group::operator!=(const Group& other) const {
	return other!=_id;
}



} // namespace Cumulus
