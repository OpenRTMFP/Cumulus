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
#include "ServerSession.h"
#include "Cookie.h"
#include "Gateway.h"

namespace Cumulus {

class HelloAttempt : public Attempt {
public:
	HelloAttempt() : pCookie(NULL),pTarget(NULL) {
	}
	~HelloAttempt() {
	}
	Cookie*		pCookie;
	Target*		pTarget;
};

class Handshake : public ServerSession {
public:
	Handshake(Gateway& gateway,Handler& handler,Entity& entity);
	~Handshake();

	void		createCookie(PacketWriter& writer,HelloAttempt& attempt,const std::string& tag,const std::string& queryUrl);
	void		commitCookie(const Poco::UInt8* value);
	void		manage();
	void		clear();
	Session*	createSession(const Poco::UInt8* cookieValue);

private:
	void		flush();
	void		flush(AESEngine::Type type);

	void		packetHandler(PacketReader& packet);
	Poco::UInt8	handshakeHandler(Poco::UInt8 id,PacketReader& request,PacketWriter& response);

	struct CompareCookies {
	   bool operator()(const Poco::UInt8* a,const Poco::UInt8* b) const {
		   return std::memcmp(a,b,COOKIE_SIZE)<0;
	   }
	};
	
	std::map<const Poco::UInt8*,Cookie*,CompareCookies> _cookies; // Cookie, in waiting of creation session
	Poco::UInt8											_certificat[77];
	Gateway&											_gateway;
};

inline void Handshake::flush() {
	ServerSession::flush(0x0b,false);
}

inline void Handshake::flush(AESEngine::Type type) {
	ServerSession::flush(0x0b,false,type);
}


} // namespace Cumulus
