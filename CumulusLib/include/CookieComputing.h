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
#include "Invoker.h"
#include <openssl/evp.h>


namespace Cumulus {

#define COOKIE_SIZE 0x40

class Handshake;
class CookieComputing : public WorkThread, private Task {
public:
	CookieComputing(Invoker& invoker,Handshake*	pHandshake, bool isMiddle);
	~CookieComputing();

	const Poco::UInt8			value[COOKIE_SIZE];
	std::vector<Poco::UInt8>	nonce;
	DH*							pDH;
	std::vector<Poco::UInt8>	initiatorKey;
	std::vector<Poco::UInt8>	initiatorNonce;
	Poco::UInt8					decryptKey[AES_KEY_SIZE];
	Poco::UInt8					encryptKey[AES_KEY_SIZE];
	std::vector<Poco::UInt8>	sharedSecret;
	

private:
	void						run();
	void						handle();
	Handshake*					_pHandshake;
	
};

} // namespace Cumulus
