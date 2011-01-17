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
#include "Client.h"
#include "Poco/Net/SocketAddress.h"
#include <vector>
#include <list>

namespace Cumulus {

class Group;
class Peer : public Client {
	friend class Group;
	friend class Session;
public:
	Peer();
	virtual ~Peer();

	const Poco::Net::SocketAddress&				address() const;
	const std::vector<Poco::Net::SocketAddress> allAddress;

	void addPrivateAddress(const Poco::Net::SocketAddress& address);
	void setPing(Poco::UInt16 ping);
	Poco::UInt16 getPing();
	bool isIn(Group& group);

private:
	void unsubscribeGroups();
	bool isIn(Group& group,std::list<Group*>::iterator& it);

	std::list<Group*>			_groups;
	Poco::UInt16				_ping;
	Poco::Net::SocketAddress	_emptyAddress;
};

inline Poco::UInt16 Peer::getPing() {
	return _ping;
}

inline const Poco::Net::SocketAddress& Peer::address() const {
	return allAddress.size()>0 ? allAddress[0] : _emptyAddress;
}


} // namespace Cumulus
