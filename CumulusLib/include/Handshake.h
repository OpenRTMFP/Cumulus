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
#include "Session.h"
#include "Cookie.h"
#include "Gateway.h"

namespace Cumulus {


class Handshake : public Session {
public:
	Handshake(Gateway& gateway,Poco::Net::DatagramSocket& socket,ServerHandler& serverHandler);
	~Handshake();

	void	createCookie(PacketWriter& writer,Cookie* pCookie);
	void	commitCookie(const Session& session);
	void manage();
	void clear();
private:
	void		packetHandler(PacketReader& packet);
	Poco::UInt8	handshakeHandler(Poco::UInt8 id,PacketReader& request,PacketWriter& response);

	// Cookie, in waiting of creation session
	std::map<std::string,Cookie*>	_cookies;

	Poco::UInt8					_certificat[77];

	Gateway&		_gateway;
};


} // namespace Cumulus
