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

#include "EdgeSession.h"
#include "Logs.h"
#include "Util.h"
#include "Poco/Format.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

EdgeSession::EdgeSession(ReceivingEngine& receivingEngine,
				SendingEngine& sendingEngine,
				UInt32 id,
				UInt32 farId,
				const Peer& peer,
				const UInt8* decryptKey,
				const UInt8* encryptKey,
				DatagramSocket& serverSocket,
				Cookie& cookie) : Session(receivingEngine,sendingEngine,id,farId,peer,decryptKey,encryptKey),_serverSocket(serverSocket),farServerId(0),_pCookie(&cookie) {
		
}

EdgeSession::~EdgeSession() {
	if(!died) {
		WARN("Session failed on the edge side : sessions are deleting");
		writer().clear(6);
		writer().write8(0x4a);
		writer() << RTMFP::TimeNow();
		writer().write8(0x0C);
		writer().write16(0);
		send();
	}
}

void EdgeSession::packetHandler(PacketReader& packet) {

	UInt8 marker = packet.read8();
	UInt16 time = packet.read16();

	if(peer.addresses.size()==0) {
		CRITIC("Session %u has no any addresses!",id);
		peer.addresses.push_front(peer.address.toString());
	} else if(peer.addresses.front()!=peer.address) {
		// Tell to server that peer address has changed!
		INFO("Session %u has changed its public address",id);
		writer().clear(6);
		writer().write8(0x89);
		writer().write16(time);
		writer().write8(0x70);
		string address = peer.address.toString();
		writer().write16(address.size() + Util::Get7BitValueSize(address.size()));
		writer() << address;
		nextDumpAreMiddle=true;
		send(farServerId,_serverSocket,_serverSocket.peerAddress(),AESEngine::EMPTY);
	}
	
	// echo time
	if((marker|0xF0) == 0xFD)
		packet.next(2);
	// Detect died!
	UInt8 type = packet.available()>0 ? packet.read8() : 0xFF;
	while(type!=0xFF && type!=0x4C) {
		packet.next(packet.read16());
		type = packet.available()>0 ? packet.read8() : 0xFF;
	}
	if(type==0x4C)
		kill();

	// Send to server
	packet.reset(0);
	writer().clear();
	writer().writeRaw(packet.current(),packet.available());

	nextDumpAreMiddle=true;
	send(farServerId,_serverSocket,_serverSocket.peerAddress(),AESEngine::EMPTY);
}

void EdgeSession::serverPacketHandler(PacketReader& packet) {
	if(!RTMFP::ReadCRC(packet)) {
		ERROR("Decrypt error on edge session %u",id);
		return;
	}
	Logs::Dump(packet,format("Request from %s",_serverSocket.peerAddress().toString()).c_str(),true);

	UInt8 marker = packet.read8();
	if(_pCookie && marker==0x0b) {
		packet.next(5);
		(UInt32&)farServerId = packet.read32();
		(UInt8&)_pCookie->response = 0x78;

		packet.reset(0);
		writer().clear(9);
		writer().write8(0x78);
		writer().next(2);
		UInt16 size = _pCookie->read(writer());
		writer().reset(10);
		writer().write16(size);
		_pCookie=NULL;
		send(AESEngine::SYMMETRIC);
		return;
	} else if((marker|0x0F) == 0x4F) {
		// echo time?
		packet.next(2);
		if((marker|0xF0) == 0xFE)
			packet.next(2);
		// Detect changing address received!
		UInt8 type = packet.available()>0 ? packet.read8() : 0xFF;
		while(type!=0xFF && type!=0x71) {
			packet.next(packet.read16());
			type = packet.available()>0 ? packet.read8() : 0xFF;
		}
		if(type==0x71) {
			string address;
			packet.readString16(address);
			if(Address(address) == peer.address) {
				if(peer.addresses.size()==0)
					CRITIC("Session %u has no any addresses!",id)
				else
					peer.addresses.pop_front();
				peer.addresses.push_front(peer.address.toString());
				DEBUG("Public address change of session %u has been commited",id);
			} else
				DEBUG("Obsolete commiting of public address change for session %u",id);
		}
	}

	packet.reset(0);
	writer().clear();
	writer().writeRaw(packet.current(),packet.available());
	send();

}

} // namespace Cumulus
