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
#include "Poco/Net/SocketAddress.h"
#include <vector>
#include <list>

namespace Cumulus {


class Peer {
	friend class Group;
public:
	Peer();
	virtual ~Peer();

	bool operator==(const Peer& other) const;
	bool operator==(const Poco::UInt8* id) const;
	bool operator!=(const Peer& other) const;
	bool operator!=(const Poco::UInt8* id) const;

	const Poco::UInt8							id[32];
	const Poco::Net::SocketAddress				address;
	const std::vector<Poco::Net::SocketAddress> privateAddress;

	void addPrivateAddress(const Poco::Net::SocketAddress& address);

	bool isIn(Group& group,std::list<Group*>::const_iterator& it=std::list<Group*>::const_iterator());

private:
	std::list<Group*>	_groups;
};

inline bool Peer::operator==(const Peer& other) const {
	return memcmp(id,other.id,32)==0;
}
inline bool Peer::operator==(const Poco::UInt8* id) const {
	return memcmp(this->id,id,32)==0;
}

inline bool Peer::operator!=(const Peer& other) const {
	return memcmp(id,other.id,32)!=0;
}
inline bool Peer::operator!=(const Poco::UInt8* id) const {
	return memcmp(this->id,id,32)!=0;
}


} // namespace Cumulus
