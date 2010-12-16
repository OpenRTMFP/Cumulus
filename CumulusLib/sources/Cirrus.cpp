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

#include "Cirrus.h"
#include "Logs.h"
#include "Util.h"
#include "Middle.h"
#include "Poco/URI.h"
#include "Poco/HexBinaryEncoder.h"
#include "Poco/Net/SocketAddress.h"


using namespace std;
using namespace Poco;
using namespace Net;

namespace Cumulus {

Cirrus::Cirrus(const string& url,Sessions& sessions) : _sessions(sessions) {
	URI uri(url);
	_url = uri.toString();
	_address = SocketAddress(uri.getHost(),uri.getPort());
}


Cirrus::~Cirrus() {
}


const BLOB& Cirrus::findPeerId(const BLOB& middlePeerId) {
	Sessions::Iterator it;
	for(it=_sessions.begin();it!=_sessions.end();++it) {
		Middle* pMiddle = (Middle*)it->second;
		if(pMiddle->middlePeerId()==middlePeerId)
			return pMiddle->peerId();
	}
	char printMiddlePeerId[65];
	MemoryOutputStream mos(printMiddlePeerId,65);
	HexBinaryEncoder(mos).write((char*)middlePeerId.begin(),32); mos.put('\0');
	ERROR("No peer id find for midde id '%s'",printMiddlePeerId);
	return middlePeerId;
}


} // namespace Cumulus
