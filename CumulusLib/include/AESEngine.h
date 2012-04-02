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
#include <openssl/aes.h>

namespace Cumulus {

#define AES_KEY_SIZE	0x20

class AESEngine
{
public:
	enum Direction {
		DECRYPT=0,
		ENCRYPT
	};
	enum Type {
		DEFAULT=0,
		EMPTY,
		SYMMETRIC
	};
	AESEngine(const Poco::UInt8* key,Direction direction);
	AESEngine(const AESEngine& other,Type type=DEFAULT);
	virtual ~AESEngine();

	AESEngine	next(Type type);
	AESEngine	next();
	void		process(const Poco::UInt8* in,Poco::UInt8* out,Poco::UInt32 size);

	const Type	type;
private:
	Direction	_direction;
	AES_KEY		_key;
	

	static AESEngine	s_aesDecrypt;
	static AESEngine	s_aesEncrypt;
};

inline AESEngine AESEngine::next() {
	return AESEngine(*this);
}

inline AESEngine AESEngine::next(Type type) {
	return AESEngine(*this,type);
}



} // namespace Cumulus
