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
#include "AESEngine.h"
#include "PacketWriter.h"
#include "RTMFP.h"
#include "Poco/RefCountedObject.h"
#include "Poco/Net/DatagramSocket.h"

namespace Cumulus {

class SendingUnit : public Poco::RefCountedObject {
public:
	SendingUnit(): packet(_buffer,sizeof(_buffer)),pAES(NULL) {
		packet.clear(6);
		packet.limit(RTMFP_MAX_PACKET_LENGTH); // set normal limit
	}
	~SendingUnit() {
		if(pAES)
			delete pAES;
	}
	Poco::UInt32				id;
	Poco::UInt32				farId;
	Poco::Net::DatagramSocket	socket;
	AESEngine*					pAES;
	Poco::Net::SocketAddress	address;
	PacketWriter				packet;
private:
	Poco::UInt8					_buffer[PACKETSEND_SIZE];
};

} // namespace Cumulus
