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

#include "RTMFPReceiving.h"
#include "RTMFPServer.h"
#include "Logs.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

RTMFPReceiving::RTMFPReceiving(RTMFPServer& server): _server(server),pPacket(NULL),id(0) {

}

RTMFPReceiving::~RTMFPReceiving() {
	if(pPacket)
		delete pPacket;
}

void RTMFPReceiving::run() {
	if(!pPacket) {
		ERROR("RTMFPReceiving on session %u with a null packet",id);
		return;
	}
	if(!RTMFP::Decode(decoder,*pPacket)) {
		ERROR("Decrypt error on session %u",id);
		return;
	}
	_server.receive(*this);
}

PacketReader* RTMFPReceiving::receive(DatagramSocket& socket) {
	this->socket = socket;
	int size = socket.receiveFrom(_buff,sizeof(_buff),address);
	if(_server.isBanned(address.host())) {
		INFO("Data rejected because client %s is banned",address.host().toString().c_str());
		return NULL;
	}
	if(size<RTMFP_MIN_PACKET_SIZE) {
		ERROR("Invalid packet");
		return NULL;
	}
	if(pPacket)
		delete pPacket;
	return pPacket = new PacketReader(_buff,size);
}



} // namespace Cumulus
