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
#include "string.h"

using namespace Poco;
using namespace Net;
using namespace std;

namespace Cumulus {

Target::Target(const SocketAddress& address,Cookie* pCookie) : address(address),publicKey(),isPeer(pCookie?true:false),peerId(),pDH(pCookie?pCookie->_pDH:NULL) {
	if(address.port()==0)
		((SocketAddress&)this->address) = SocketAddress(address.host(),RTMFP_DEFAULT_PORT);
	if(isPeer) {
		memcpy((UInt8*)publicKey,&pCookie->_nonce[11],KEY_SIZE);
		((vector<UInt8>&)pCookie->_nonce)[9] = 0x1D;
		EVP_Digest(&pCookie->_nonce[7],pCookie->_nonce.size()-7,(UInt8*)id,NULL,EVP_sha256(),NULL);
		((vector<UInt8>&)pCookie->_nonce)[9] = 0x0D;
		pCookie->_pDH = NULL;
	}
}


Target::~Target() {
	if(pDH)
		RTMFP::EndDiffieHellman(pDH);
}



} // namespace Cumulus
