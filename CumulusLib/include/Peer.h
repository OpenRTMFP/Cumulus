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
#include "Clients.h"
#include "Address.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/DatagramSocket.h"
#include <vector>
#include <list>

#include "AESEngine.h"

namespace Cumulus {

class Group;
class Peer : public Client {
	friend class Group;
public:

	Peer();
	virtual ~Peer();

	Poco::Net::SocketAddress		address;
	const std::vector<Address>		privateAddress;

	const bool						connected;

	void setFlowWriter(FlowWriter* pWriter);
	void setPrivateAddress(const std::list<Address>& address);
	void setPing(Poco::UInt16 ping);
	void unsubscribeGroups();

	Poco::UInt16 getPing() const;
	bool isIn(Group& group) const;

private:
	bool isIn(Group& group,std::list<Group*>::iterator& it);

	std::list<Group*>			_groups;
	Poco::UInt16				_ping;
};

inline void Peer::setFlowWriter(FlowWriter* pWriter){
	_pFlowWriter = pWriter;
}

inline Poco::UInt16 Peer::getPing() const {
	return _ping;
}

} // namespace Cumulus
