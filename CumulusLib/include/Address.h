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

namespace Cumulus {


class Address {
public:
	Address();
	Address(const std::string& host,Poco::UInt16 port);
	Address(const std::string& address);
	virtual ~Address();

	Address& operator=(const Address& other);
	bool operator==(const Address& other) const;
	bool operator==(const Poco::Net::SocketAddress& address) const;
	bool operator!=(const Address& other) const;
	bool operator!=(const Poco::Net::SocketAddress& address) const;

	const std::vector<Poco::UInt8>	host;
	const Poco::UInt16				port;
private:
	void buildHost(const std::string& host);
};

inline bool Address::operator==(const Address& other) const {
	return other.host==host && other.port==port;
}
inline bool Address::operator!=(const Address& other) const {
	return other.host!=host || other.port!=port;
}

inline bool Address::operator!=(const Poco::Net::SocketAddress& address) const {
	return !(operator==(address));
}


} // namespace Cumulus
