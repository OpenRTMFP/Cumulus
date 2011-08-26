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
#include <string.h>

using namespace std;
using namespace Poco;

namespace Cumulus {

AESEngine::AESEngine(const UInt8* key,Direction direction) : _direction(direction) {
	if(_direction==DECRYPT)
		AES_set_decrypt_key(key, 0x80,&_key);
	else
		AES_set_encrypt_key(key, 0x80,&_key);
}


AESEngine::~AESEngine() {
}

void AESEngine::process(const UInt8* in,UInt8* out,UInt32 size) {
	UInt8	iv[AES_KEY_SIZE];
	memset(iv,0,sizeof(iv));
	AES_cbc_encrypt(in, out,size,&_key,iv, _direction);
}





} // namespace Cumulus
