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
#include "Entity.h"
#include "RTMFP.h"
#include "Poco/Net/SocketAddress.h"

namespace Cumulus {

class Cookie;
class Target : public Entity {
public:
	Target(const Poco::Net::SocketAddress& address,Cookie* pCookie=NULL);
	virtual ~Target();

	const Poco::Net::SocketAddress	address;
	const bool						isPeer;
	const Poco::UInt8				peerId[ID_SIZE];

	const Poco::UInt8				publicKey[KEY_SIZE+4];
	DH*								pDH;
};

} // namespace Cumulus
