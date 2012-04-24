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

#include "RTMFPSending.h"
#include "RTMFP.h"
#include "Logs.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

RTMFPSending::RTMFPSending(): packet(_buffer,sizeof(_buffer)),id(0),farId(0) {
	packet.clear(6);
	packet.limit(RTMFP_MAX_PACKET_LENGTH); // set normal limit
}

RTMFPSending::~RTMFPSending() {

}

void RTMFPSending::run() {
	RTMFP::Encode(encoder,packet);
	RTMFP::Pack(packet,farId);
	try {
		if(socket.sendTo(packet.begin(),(int)packet.length(),address)!=packet.length())
			ERROR("Socket sending error on session %u : all data were not sent",id);
	} catch(Exception& ex) {
		 WARN("Socket sending error on session %u : %s",id,ex.displayText().c_str());
	} catch(exception& ex) {
		 WARN("Socket sending error on session %u : %s",id,ex.what());
	} catch(...) {
		 WARN("Socket sending unknown error on session %u",id);
	}
}




} // namespace Cumulus
