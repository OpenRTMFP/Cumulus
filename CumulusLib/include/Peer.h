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

namespace Cumulus {

class Group;
class Peer : public Client {
	friend class Group;
public:

	Peer();
	virtual ~Peer();

	const Poco::Net::SocketAddress	address;
	const std::list<Address>		addresses;

	const bool						connected;
	const Poco::UInt16				ping;

	void setFlowWriter(FlowWriter* pWriter);
	void unsubscribeGroups();

	bool isIn(Group& group) const;

private:
	bool isIn(Group& group,std::list<Group*>::iterator& it);

	std::list<Group*>			_groups;
};

inline void Peer::setFlowWriter(FlowWriter* pWriter){
	_pFlowWriter = pWriter;
}

} // namespace Cumulus
