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
#include "AESEngine.h"
#include "Sessions.h"


namespace Cumulus {


class Cirrus
{
public:
	Cirrus(const Poco::Net::SocketAddress& address,const std::string& pathAndQuery,Sessions& sessions);
	virtual ~Cirrus();

	const Peer&						findPeer(const Peer& middlePeer);

	const std::string&				url();
	const Poco::Net::SocketAddress&	address();

private:
	Sessions&					_sessions;
	std::string					_url;
	Poco::Net::SocketAddress	_address;
};

inline const std::string& Cirrus::url() {
	return _url;
}

inline const Poco::Net::SocketAddress& Cirrus::address() {
	return _address;
}


} // namespace Cumulus
