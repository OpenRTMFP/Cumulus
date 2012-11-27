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

#include "CookieComputing.h"
#include "Handshake.h"
#include "Util.h"
#include "Logs.h"
#include "Poco/RandomStream.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

CookieComputing::CookieComputing(Invoker& invoker,Handshake* pHandshake): _pHandshake(pHandshake),value(),Task(invoker),pDH(NULL),nonce(pHandshake ? 7 : 73) {
	RandomInputStream().read((char*)value,COOKIE_SIZE);
	if(!pHandshake) { // Target type
		memcpy(&nonce[0],"\x03\x1A\x00\x00\x02\x1E\x00\x41\x0E",9);
		RandomInputStream().read((char*)&nonce[9],64);
	} else
		memcpy(&nonce[0],"\x03\x1A\x00\x00\x02\x1E\x00",7);
}

CookieComputing::~CookieComputing() {
	if(_pHandshake && pDH)
		RTMFP::EndDiffieHellman(pDH);
}

void CookieComputing::run() {
	// First execution is for the DH computing if pDH == null, else it's to compute Diffie-Hellman keys
	if(!pDH) {
		pDH = RTMFP::BeginDiffieHellman(nonce);
		return;
	}
	// Compute Diffie-Hellman secret
	vector<UInt8> sharedSecret;
	RTMFP::ComputeDiffieHellmanSecret(pDH,&initiatorKey[0],initiatorKey.size(),sharedSecret);

	DEBUG("Shared Secret : %s",Util::FormatHex(&sharedSecret[0],sharedSecret.size()).c_str());

	// Compute Keys
	RTMFP::ComputeAsymetricKeys(sharedSecret,&initiatorNonce[0],initiatorNonce.size(),&nonce[0],nonce.size(),decryptKey,encryptKey);
	waitHandle();
}

void CookieComputing::handle() {
	Session* pSession = _pHandshake->createSession(value);
	if(pSession) {
		duplicate();
		pSession->peer.pinObject<CookieComputing>(*this);
	}
}


} // namespace Cumulus
