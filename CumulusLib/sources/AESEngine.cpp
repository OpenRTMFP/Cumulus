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

#include "AESEngine.h"
#include "RTMFP.h"
#include <string.h>

using namespace std;
using namespace Poco;

namespace Cumulus {

AESEngine AESEngine::s_aesDecrypt(RTMFP_SYMETRIC_KEY,AESEngine::DECRYPT);
AESEngine AESEngine::s_aesEncrypt(RTMFP_SYMETRIC_KEY,AESEngine::ENCRYPT);

AESEngine::AESEngine() : type(EMPTY),_direction(DECRYPT) {
}

AESEngine::AESEngine(const UInt8* key,Direction direction) : type(key ? DEFAULT : EMPTY),_direction(direction) {
	if(!key)
		return;
	if(_direction==DECRYPT)
		AES_set_decrypt_key(key, 0x80,&_key);
	else
		AES_set_encrypt_key(key, 0x80,&_key);
}

AESEngine::AESEngine(const AESEngine& other,Type type) : type(other.type==EMPTY ? EMPTY : type),_key(other._key),_direction(other._direction) {
}

AESEngine::AESEngine(const AESEngine& other) : type(other.type),_key(other._key),_direction(other._direction) {
}

AESEngine& AESEngine::operator=(const AESEngine& other) {
	(Type&)type = other.type;
	_key = other._key;
	_direction = other._direction;
	return *this;
}


AESEngine::~AESEngine() {
}

void AESEngine::process(const UInt8* in,UInt8* out,UInt32 size) {
	if(type==EMPTY)
		return;
	if(type==SYMMETRIC) {
		if(_direction==DECRYPT)
			s_aesDecrypt.process(in,out,size);
		else
			s_aesEncrypt.process(in,out,size);
		return;
	}
	UInt8	iv[AES_KEY_SIZE];
	memset(iv,0,sizeof(iv));
	AES_cbc_encrypt(in, out,size,&_key,iv, _direction);
}





} // namespace Cumulus
