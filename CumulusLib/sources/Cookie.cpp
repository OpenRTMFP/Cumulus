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

#include "Cookie.h"
#include "Util.h"
#include "Poco/RandomStream.h"
#include "string.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

Cookie::Cookie(const string& tag,const string& queryUrl) : tag(tag),value(),response(0x78),_pDH(NULL),_nonce(KEY_SIZE+11),pTarget(NULL),id(0),queryUrl(queryUrl),_writer(_buffer,sizeof(_buffer)) {
	RandomInputStream().read((char*)value,COOKIE_SIZE);
	memcpy(&_nonce[0],"\x03\x1A\x00\x00\x02\x1E\x00\x81\x02\x0D\x02",11);
	_pDH = RTMFP::BeginDiffieHellman(&_nonce[11]);
}

Cookie::Cookie(const string& tag,Target& target) : tag(tag),value(),response(0x78),_nonce(73),_pDH(target.pDH),pTarget(&target),id(0),_writer(_buffer,sizeof(_buffer)) {
	RandomInputStream().read((char*)value,COOKIE_SIZE);
	memcpy(&_nonce[0],"\x03\x1A\x00\x00\x02\x1E\x00\x41\x0E",9);
	RandomInputStream().read((char*)&_nonce[9],64);
}

Cookie::~Cookie() {
	if(!pTarget && _pDH)
		RTMFP::EndDiffieHellman(_pDH);
}

void Cookie::computeKeys(const UInt8* initiatorKey,UInt16 initKeySize,const UInt8* initiatorNonce,UInt16 initNonceSize,UInt8* decryptKey,UInt8* encryptKey) {
	// Compute Diffie-Hellman secret
	UInt8 sharedSecret[KEY_SIZE];
	RTMFP::ComputeDiffieHellmanSecret(_pDH,initiatorKey,initKeySize,sharedSecret);
	//DEBUG("Shared Secret : %s",Util::FormatHex(sharedSecret,sizeof(sharedSecret)).c_str());
	// Compute Keys
	RTMFP::ComputeAsymetricKeys(sharedSecret,initiatorNonce,initNonceSize,&_nonce[0],_nonce.size(),decryptKey,encryptKey);
	if(pTarget) {
		((vector<UInt8>&)pTarget->initiatorNonce).resize(initNonceSize);
		memcpy(&((vector<UInt8>&)pTarget->initiatorNonce)[0],initiatorNonce,initNonceSize);
		memcpy((UInt8*)pTarget->sharedSecret,sharedSecret,KEY_SIZE);
	}
}


void Cookie::write() {
	if(_writer.length()==0) {
		_writer.write32(id);
		_writer.write7BitLongValue(_nonce.size());
		_writer.writeRaw(&_nonce[0],_nonce.size());
		_writer.write8(0x58);
	}
}

UInt16 Cookie::read(PacketWriter& writer) {
	writer.writeRaw(_writer.begin(),_writer.length());
	return _writer.length();
}


} // namespace Cumulus
