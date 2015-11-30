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

#include "Target.h"
#include "RTMFP.h"
#include "Cookie.h"
#include <openssl/evp.h>
#include <cstring>

using namespace Poco;
using namespace Net;
using namespace std;

namespace Cumulus {

Target::Target(const SocketAddress& address,Cookie* pCookie) : address(address),isPeer(pCookie?true:false),peerId(),pDH(pCookie?pCookie->_pCookieComputing->pDH:NULL) {
	if(address.port()==0)
		((SocketAddress&)this->address) = SocketAddress(address.host(),RTMFP_DEFAULT_PORT);
	if(isPeer) {
		((vector<UInt8>&)publicKey).resize(pCookie->_pCookieComputing->nonce.size()-7);
		memcpy((void*)&publicKey[0],&pCookie->_pCookieComputing->nonce[7],publicKey.size());
		((vector<UInt8>&)publicKey)[2] = 0x1D;
		EVP_Digest(&publicKey[0],publicKey.size(),(UInt8*)id,NULL,EVP_sha256(),NULL);
		pCookie->_pCookieComputing->pDH = NULL;
	}
}


Target::~Target() {
	if (!isPeer && pDH) {
		RTMFP::EndDiffieHellman(pDH);
		pDH = NULL;
	}
}



} // namespace Cumulus
