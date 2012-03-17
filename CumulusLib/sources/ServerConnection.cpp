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

#include "ServerConnection.h"
#include "Util.h"
#include "Poco/RandomStream.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {


ServerConnection::ServerConnection(Handler& handler,Session& handshake) : ServerSession(0,0,Peer(handler),NULL,NULL,(Invoker&)handler),_handshake(handshake),_connected(false) {
	RandomInputStream().read((char*)peer.id,ID_SIZE);
}

ServerConnection::~ServerConnection() {
	kill();
}

void ServerConnection::connect(const SocketAddress& publicAddress) {
	INFO("RTMFP server connection attempt");
	PacketWriter& packet(writer());
	packet.write8(0x41);
	UInt16 pos = packet.position();
	packet.next(2);
	packet.writeAddress(publicAddress,false);
	packet.reset(pos);
	packet.write16(packet.length()-packet.position()-2);
	flush();
}

void ServerConnection::disconnect() {
	(bool&)died=false;
	_connected=false;
	PacketWriter& packet(writer());
	packet.write8(0x45);
	packet.write16(0);
	flush();
}

void ServerConnection::manage() {
	map<string,P2PHandshakerAddress>::iterator it = _p2pHandshakers.begin();
	while(it!=_p2pHandshakers.end()) {
		if(it->second.obsolete())
			_p2pHandshakers.erase(it++);
		else
			++it;
	}
}

void ServerConnection::createSession(EdgeSession& session,const string& url) {
	PacketWriter& packet(writer());
	packet.write8(0x39);
	packet.write16(5+COOKIE_SIZE+Util::Get7BitValueSize(url.size())+url.size());
	packet.write32(session.id);
	packet.write8(COOKIE_SIZE);
	packet.writeRaw(session.peer.id,ID_SIZE);
	packet.writeRaw(peer.id,ID_SIZE);
	packet << url;
	packet << session.peer.address.toString();
	flush();
}

void ServerConnection::sendP2PHandshake(const string& tag,const SocketAddress& address,const UInt8* peerIdWanted) {
	_p2pHandshakers[tag] = address;
	PacketWriter& packet(writer());
	packet.write8(0x30);
	packet.write16(3+ID_SIZE+tag.size());
	packet.write8(0x22);
	packet.write8(0x21);
	packet.write8(0x0F);
	packet.writeRaw(peerIdWanted,ID_SIZE);
	packet.writeRaw(tag);
	flush();
}

void ServerConnection::packetHandler(PacketReader& packet) {

	UInt8 marker = packet.read8();
	if(marker!=0x0b) {
		ERROR("ServerConnection with an unknown %u marker, it should be 0x0b",marker);
		return;
	}

	packet.next(2);
	UInt8 id = packet.read8();
	switch(id) {
		case 0x71: {
			packet.next(2);
			string tag;
			packet.readString8(tag);
			map<string,P2PHandshakerAddress>::iterator it = _p2pHandshakers.find(tag);
			if(it==_p2pHandshakers.end()) {
				ERROR("Unknown ServerConnection tag %s on P2P handshake",tag.c_str());
				break;
			}

			(SocketAddress&)_handshake.peer.address = it->second;
			packet.reset(0);
			PacketWriter writer(packet.current(),packet.available()+16); // +16 for futur 0xFFFF padding
			writer.clear(packet.available());
			_handshake.send(writer);
			_p2pHandshakers.erase(it);

			break;
		}
		case 0x40: {
			if(!_connected) {
				// Edge hello response
				_connected=true;
				return;
			}
			// Edge keepalive
			PacketWriter& packet(writer());
			packet.write8(0x41);
			packet.write16(0);
			flush();
			INFO("Keepalive RTMFP server");
			break;
		}
		case 0x45: {
			// Server is death!
			(bool&)died=true;
			break;
		}
		default:
			ERROR("Unkown ServerConnection packet id %u",id);
	}


}


} // namespace Cumulus
