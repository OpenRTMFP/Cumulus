/* 
	Copyright 2010 cumulus.dev@gmail.com
 
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
#include "Middle.h"
#include "Poco/Net/DatagramSocket.h"

namespace Cumulus {


class Handshake : public Session {
public:
	Handshake(Poco::Net::DatagramSocket& socket,const std::string& listenCirrusUrl="");
	~Handshake();

	Poco::UInt32	nextSessionId;
	Session*		getNewSession();
private:
	void		packetHandler(PacketReader& packet,Poco::Net::SocketAddress& sender);
	Poco::UInt8	handshakeHandler(Poco::UInt8 id,PacketReader& request,PacketWriter& response);

	Session* createSession(Poco::UInt32 id,Poco::UInt32 farId,const std::string& url,const Poco::UInt8* decryptKey,const Poco::UInt8* encryptKey);


	// For the man in the middle mode
	Middle*	_pOneMiddle;

	// Cookie, in waiting of creation session
	std::map<std::string,Cookie*> _cookies;

	// New Session (session acceptation)
	Session*		_pNewSession;
	std::string		_listenCirrusUrl;

	char		_certificat[77];
	std::string _signature;

};


} // namespace Cumulus
