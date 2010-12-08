/* 
	Copyright 2010 cumulus.dev@gmail.com
 
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
#include "Util.h"
#include "Logs.h"
#include "FlowNetConnection.h"
#include "FlowNetStream.h"
#include "FlowNull.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Session::Session(UInt32 id,
				 UInt32 farId,
				 const UInt8* peerId,
				 const SocketAddress& peerAddress,
				 const string& url,
				 const UInt8* decryptKey,
				 const UInt8* encryptKey,
				 DatagramSocket& socket,
				 Database& database) : 
	_id(id),_farId(farId),_socket(socket),
	_aesDecrypt(decryptKey,AESEngine::DECRYPT),_aesEncrypt(encryptKey,AESEngine::ENCRYPT),_url(url),_database(database),_peerId(peerId,peerId?32:0),_peerAddress(peerAddress) {

		
}


Session::~Session() {
	// delete flows
	map<UInt8,Flow*>::const_iterator it;
	for(it=_flows.begin();it!=_flows.end();++it)
		delete it->second;
	_flows.clear();
}

Flow& Session::flow(Poco::UInt8 id) {
	if(_flows.find(id)==_flows.end())
		_flows[id] = createFlow(id);
	return *_flows[id];
}

void Session::p2pHandshake(const SocketAddress& peerAddress) {
	DEBUG("Peer newcomer address send to peer '%u' connected",id());
	PacketWriter packet(9);
	packet << RTMFP::Timestamp();
	packet.write8(0x0F);
	{
		PacketWriter content = packet;
		content.skip(2);
		content.write8(_peerId.size()+2);
		content.write8(_peerId.size()+1);

		content.write8(0x0F);
		content.writeRaw(_peerId.begin(),_peerId.size());

		content.write8(0x02);
		content.writeAddress(peerAddress);

		content.writeRandom(16); // tag
	}

	packet.write16(packet.size()-packet.position()-2);
	send(0x4e,packet);
}


void Session::send(UInt8 marker,PacketWriter packet) {
	packet.reset(6);
	packet << marker;
	packet << RTMFP::Timestamp();

	if(Logs::Dump()) {
		printf("Response:\n");
		Util::Dump(packet,6,Logs::DumpFile());
	}

	RTMFP::Encode(_aesEncrypt,packet);

	RTMFP::Pack(packet,_farId);

	try {
		// TODO remake? without retry (but flow)
		bool retry=false;
		while(_socket.sendTo(packet.begin(),packet.size(),_peerAddress)!=packet.size()) {
			if(retry) {
				ERROR("Socket send error on session '%u' : all data were not sent",_id);
				break;
			}
			retry = true;
		}
	} catch(Exception& ex) {
		ERROR("Socket send error on session '%u' : %s",_id,ex.displayText().c_str());
	}
}

void Session::packetHandler(PacketReader& packet) {

	// Read packet
	UInt8 marker = packet.next8();
	if(marker!=0x8d &&  marker!=0x0d) {
		ERROR("Packet marker unknown : %02x",marker);
		return;
	}

	UInt16 sendTime = packet.next16();
	UInt16 timeEcho = packet.next16();

	// Begin a possible response
	PacketWriter packetOut(9); // 9 for future marker, timestamp, crc 2 bytes and id session 4 bytes
	packetOut << sendTime; // echo time response

	UInt8 type = packet.available()>0 ? packet.next8() : 0xFF;
	bool answer = false;

	// Can have nested queries
	while(type!=0xFF) {
		UInt16 size = packet.next16();
		int idResponse = 0;

		{
			PacketReader request(packet.current(),size);

			PacketWriter response = packetOut;
			response.skip(3); // skip the futur possible id and length response

			switch(type) {
				case 0x4c :
					/// Session death!
					// TODO
					break;
				case 0x01 :
					/// KeepAlive
					keepaliveHandler();
					idResponse = 0x41;
					break;
				case 0x18 : {
					/// TODO This response is sent when we answer with a not Acknowledgment message.
					// It contains the id flow, and means that we must replay the last message
					// Be carreful : it can loop indefinitely, so change the marker for 09 (it means flow exception or close, I don't know)
					// UInt8 idFlow= request.next8();
					break;
				}
				case 0x51 : {
					/// Acknowledgment 
					UInt8 idFlow= request.next8();
					bool ack = request.next8()==0x7f;
					UInt8 stage = request.next8();
					flow(idFlow).acknowledgment(stage,ack);
					/// replay the flow is not ack
					if(!ack) {
						idResponse = 0x18;
						response.write8(idFlow);
					}
					break;
				}
				case 0x10 : {
					/// Request
					
					UInt8 firstFlag = request.next8(); // Unknown, is 0x80 or 0x00
					UInt8 idFlow= request.next8();
					UInt8 stage = request.next8();
					UInt8 secondFlag = request.next8(); // Unknown, is 0x01 in general

					// Write Acknowledgment (nested in response)
					packetOut.write8(0x51);
					packetOut.write16(3);
					packetOut.write8(idFlow);
					response.skip(6);
					answer = true;

					// Prepare response
					response.write8(firstFlag);
					response.write8(idFlow);
					response.write8(stage);
					response.write8(secondFlag);
					
					// Process request
					idResponse = flow(idFlow).request(stage,request,response);
					
					packetOut.write8(idResponse<0 ? 0x00 : 0x3f); // not ack or ack
					packetOut.write8(stage);
					
					break;
				}
				default :
					ERROR("Request type '%02x' unknown",type);
			}

			if(idResponse<=0)
				response.clear();
		}
		
		if(idResponse>0) {
			answer = true;
			packetOut.write8(idResponse);
			int len = packetOut.size()-packetOut.position()-2;
			if(len<0)
				len = 0;
			packetOut.write16(len);
			packetOut.skip(len);
		}
		
		// Next
		packet.skip(size);
		type = packet.available()>0 ? packet.next8() : 0xFF;
	}

	if(answer)
		send(0x4e,packetOut);
}


void Session::keepaliveHandler() {
	// TODO ?
}


// Don't must return a null value!
Flow* Session::createFlow(UInt8 id) {
	switch(id) {
		case 0x02 :
			// NetConnection
			return new FlowNetConnection(id,_peerId,_peerAddress,_database);
		case 0x03 :
			// NetStream on NetGroup 
			return new FlowNetStream(id,_peerId,_peerAddress,_database);
		default :
			ERROR("Flow id '%02x' unknown",id);	
	}
	return new FlowNull(id,_peerId,_peerAddress,_database);
}




} // namespace Cumulus
