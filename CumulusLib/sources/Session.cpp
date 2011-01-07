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
#include "Util.h"
#include "Logs.h"
#include "FlowConnection.h"
#include "FlowGroup.h"
#include "FlowNull.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Session::Session(UInt32 id,
				 UInt32 farId,
				 const Peer& peer,
				 const string& url,
				 const UInt8* decryptKey,
				 const UInt8* encryptKey,
				 DatagramSocket& socket,
				 ServerData& data) : 
	_id(id),_farId(farId),_socket(socket),
	_aesDecrypt(decryptKey,AESEngine::DECRYPT),_aesEncrypt(encryptKey,AESEngine::ENCRYPT),_url(url),_data(data),_peer(peer),_die(false),_timeSent(0),_packetOut(_buffer,MAX_SIZE_MSG) {

}


Session::~Session() {
	// delete flows
	map<UInt8,Flow*>::const_iterator it;
	for(it=_flows.begin();it!=_flows.end();++it)
		delete it->second;
	_flows.clear();
}

PacketWriter& Session::writer() {
	_packetOut.clear(11);  // 11 for future crc 2 bytes, id session 4 bytes, marker 1 byte, timestamp 2 bytes and timecho 2 bytes 
	return _packetOut;
}

void Session::manage() {
	if(die())
		return;
	// TODO _timeUpdate "update this time and if it's more than a laps time reference, delete session!"
	map<UInt8,Flow*>::const_iterator it;
	for(it=_flows.begin();it!=_flows.end();++it) {
//		it->second->manage();
	}
}

void Session::fail() {
	PacketWriter& packet = writer();
	packet.write8(0x0C);
	packet.write16(0x00);
	send();
}

Flow& Session::flow(Poco::UInt8 id) {
	if(_flows.find(id)==_flows.end())
		_flows[id] = createFlow(id);
	return *_flows[id];
}

void Session::p2pHandshake(const Peer& peer) {
	DEBUG("Peer newcomer address send to peer '%u' connected",id());
	PacketWriter& packetOut = writer();
	packetOut.write8(0x0F);
	{
		PacketWriter content(packetOut);
		content.skip(2);
		content.write8(34);
		content.write8(33);

		content.write8(0x0F);
		content.writeRaw(_peer.id,32);

		content.write8(0x02);
		content.writeAddress(peer.address);

		content.writeRandom(16); // tag
	}

	packetOut.write16(packetOut.size()-packetOut.position()-2);
	send();
}

void Session::send(bool symetric) {
	_packetOut.reset(6);

	bool timeEcho = true;

	// After 30 sec, send packet without echo time
	if(symetric || _recvTimestamp.isElapsed(30000000)) {
		timeEcho = false;
		_packetOut.clip(2);
	}

	UInt8 marker = symetric ? 0x0b : (timeEcho ? 0x4e : 0x4a);

	_packetOut << marker;
	_packetOut << RTMFP::TimeNow();
	if(timeEcho)
		_packetOut.write16(_timeSent+RTMFP::Time(_recvTimestamp.elapsed()));

	if(Logs::Dump()) {
		printf("Response:\n");
		Util::Dump(_packetOut,6,Logs::DumpFile());
	}

	if(symetric)
		RTMFP::Encode(_packetOut);
	else
		RTMFP::Encode(_aesEncrypt,_packetOut);

	RTMFP::Pack(_packetOut,_farId);

	try {
		// TODO remake? without retry (but flow)
		bool retry=false;
		while(_socket.sendTo(_packetOut.begin(),_packetOut.size(),_peer.address)!=_packetOut.size()) {
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

	_recvTimestamp.update();

	// Read packet
	UInt8 marker = packet.next8();
	
	_timeSent = packet.next16();

	// with time echo
	if((marker|0xF0) == 0xFD) {
		UInt16 ping = RTMFP::Time(_recvTimestamp.epochMicroseconds())-packet.next16();
		DEBUG("Ping : %hu",ping);
	}

	if(marker!=0x8d &&  marker!=0x0d && marker!=0x89 &&  marker!=0x09)
		WARN("Packet marker unknown : %02x",marker);


	// Begin a possible response
	PacketWriter& packetOut = writer();

	UInt8 type = packet.available()>0 ? packet.next8() : 0xFF;
	bool answer = false;

	// Can have nested queries
	while(type!=0xFF) {
		UInt16 size = packet.next16();
		int idResponse = 0;

		{
			PacketReader request(packet.current(),size);

			PacketWriter response(packetOut);
			response.skip(3); // skip the futur possible id and length response

			switch(type) {
				case 0x4c :
					/// Session death!
					_die = true;
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
				case 0x11 : // "request in request" case
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
		send();
}


void Session::keepaliveHandler() {
	// TODO ?
}


// Don't must return a null value!
Flow* Session::createFlow(UInt8 id) {
	switch(id) {
		case 0x02 :
			// NetConnection
			return new FlowConnection(id,_peer,_data);
		case 0x03 :
			// NetStream on NetGroup 
			return new FlowGroup(id,_peer,_data);
		default :
			ERROR("Flow id '%02x' unknown",id);	
	}
	return new FlowNull(id,_peer,_data);
}




} // namespace Cumulus
