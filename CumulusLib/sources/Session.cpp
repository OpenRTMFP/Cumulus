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

#include "Session.h"
#include "Logs.h"
#include "Poco/Format.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Session::Session(UInt32 id,
				 UInt32 farId,
				 const Peer& peer,
				 const UInt8* decryptKey,
				 const UInt8* encryptKey) : 
		died(false),checked(false),id(id),farId(farId),peer(peer),aesDecrypt(decryptKey,AESEngine::DECRYPT),aesEncrypt(encryptKey,AESEngine::ENCRYPT),_pSocket(NULL),middleDump(false) {

}


Session::~Session() {

}

void Session::setEndPoint(Poco::Net::DatagramSocket& socket,const Poco::Net::SocketAddress& address) {
	if(_pSocket!=&socket)
		_pSocket=&socket;
	(SocketAddress&)peer.address = address;
}

void Session::receive(PacketReader& packet) {
	if(!decode(packet)) {
		ERROR("Decrypt error on session %u",id);
		return;
	}

	Logs::Dump(packet,format("Request from %s",peer.address.toString()).c_str(),middleDump);
	packetHandler(packet);
}

void Session::send(PacketWriter& packet) {
	if(!_pSocket) {
		ERROR("Impossible to send on a null socket for session %u",id);
		return;
	}
	send(packet,farId,*_pSocket,peer.address);
}

void Session::send(PacketWriter& packet,UInt32 farId,DatagramSocket& socket,const SocketAddress& receiver) {
	Logs::Dump(packet,6,format("Response to %s",receiver.toString()).c_str(),middleDump);

	encode(packet);
	RTMFP::Pack(packet,farId);

	try {
		if(socket.sendTo(packet.begin(),(int)packet.length(),receiver)!=packet.length())
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
