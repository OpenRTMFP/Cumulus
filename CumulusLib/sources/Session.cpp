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
#include "Poco/URI.h"
#include "string.h"

#define FLOW_CONNECTION 0x02

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Session::Session(UInt32 id,
				 UInt32 farId,
				 const Peer& peer,
				 const UInt8* decryptKey,
				 const UInt8* encryptKey,
				 DatagramSocket& socket,
				 ServerData& data,
				 ClientHandler* pClientHandler) : 
	_id(id),_farId(farId),_socket(socket),_pClientHandler(pClientHandler),_testDecode(false),
	_aesDecrypt(decryptKey,AESEngine::DECRYPT),_aesEncrypt(encryptKey,AESEngine::ENCRYPT),_data(data),_peer(peer),_die(false),_failed(false),_timesFailed(0),_timeSent(0),_packetOut(_buffer,MAX_SIZE_MSG),_timesKeepalive(0),_clientAccepted(false) {

}


Session::~Session() {
	// delete flows
	map<UInt8,Flow*>::const_iterator it;
	for(it=_flows.begin();it!=_flows.end();++it)
		delete it->second;
	_flows.clear();
	kill();
	if(_clientAccepted && _pClientHandler)
		WARN("onDisconnect handler has not been called on the session '%u'",_id);
}

bool Session::decode(PacketReader& packet,const SocketAddress& sender) {
	((SocketAddress&)_peer.address) = sender;
	bool result = RTMFP::Decode(_aesDecrypt,packet);
	if(result)
		_testDecode = true;
	return result;
}

void Session::kill() {
	if(_die)
		return;
	if(_clientAccepted && _pClientHandler) {
		_pClientHandler->onDisconnection(_peer);
		_pClientHandler = NULL;
	}
	_die=true;
}

PacketWriter& Session::writer() {
	_packetOut.clear(11);  // 11 for future crc 2 bytes, id session 4 bytes, marker 1 byte, timestamp 2 bytes and timecho 2 bytes 
	return _packetOut;
}

void Session::manage() {
	if(die())
		return;

	// After 6 mn we considerate than the session has failed
	if(_recvTimestamp.isElapsed(360000000))
		setFailed("Timeout no client message");
	// To accelerate the deletion of peer ghost (mainly for netgroup efficient), starts a keepalive server after 2 mn
	if(!_failed && _recvTimestamp.isElapsed(120000000))
		keepAlive();

	if(_failed) {
		fail(); // send fail message in hoping to trigger the death message
		return;
	}
	
	map<UInt8,Flow*>::iterator it=_flows.begin();
	while(it!=_flows.end()) {
		Flow& flow = *it->second;

		if(flow.consumed()) {
			delete it->second;
			_flows.erase(it++);
			continue;
		}

		PacketWriter& response = writer();
		try {
			if(flow.lastResponse(response))
				send();
		} catch(const Exception& ex) {
			fail(ex.what());
			return;
		}
		++it;
	}
}

void Session::keepAlive() {
	DEBUG("Keepalive server");
	if(_timesKeepalive==10) {
		setFailed("Timeout keepalive attempts");
		return;
	}
	++_timesKeepalive;
	PacketWriter& packet = writer();
	packet.write8(0x01);
	packet.write16(0x00);
	send();
}

void Session::setFailed(const string& msg) {
	if(_failed)
		return;
	_failed=true;
	if(_pClientHandler && _clientAccepted)
		_pClientHandler->onFailed(_peer,msg);
	_peer.unsubscribeGroups();
	WARN("Session failed on the server side : %s",msg.c_str());
}

void Session::fail() {
	if(_die)
		WARN("Fail perhaps useless because session is already dead");
	if(!_failed) {
		WARN("Here flag failed should be put (with setFailed method), fail() method just allows the fail packet senging");
		_failed=true;
	}
	++_timesFailed;
	PacketWriter& packet = writer();
	packet.write8(0x0C);
	packet.write16(0x00);
	send();
	// After 6 mn we can considerated than the session is died!
	if(_timesFailed==10 || _recvTimestamp.isElapsed(360000000))
		kill();
}

Flow& Session::flow(Poco::UInt8 id,bool canCreate) {
	if(_flows.find(id)==_flows.end()) {
		if(!canCreate)
			id=0; // for force FlowNull returning
		_flows[id] = createFlow(id);
	}
	return *_flows[id];
}

void Session::p2pHandshake(const SocketAddress& address,const std::string& tag,Session* pSession) {

	DEBUG("Peer newcomer address send to peer '%u' connected",id());
	PacketWriter& packetOut = writer();
	packetOut.write8(0x0F);
	{
		PacketWriter content(packetOut,2);
		content.write8(34);
		content.write8(33);

		content.write8(0x0F);
		content.writeRaw(_peer.id,32);


		if(pSession) {
			map<string,UInt8>::iterator it =	_p2pHandshakeAttemps.find(tag);
			if(it==_p2pHandshakeAttemps.end()) {
				it = _p2pHandshakeAttemps.insert(pair<string,UInt8>(tag,0)).first;
				// If two clients are on the same lan, starts with private address
				if(memcmp(address.addr(),peer().address.addr(),address.length())==0 && pSession->peer().privateAddress.size()>0)
					it->second=1;
			}
			
			if(it->second==0)
				content.writeAddress(address,true);
			else
				content.writeAddress(pSession->peer().privateAddress[it->second-1],false);
			++it->second;
			if(it->second > pSession->peer().privateAddress.size())
				it->second=0;
		} else
			content.writeAddress(address,true);


		content.writeRaw(tag);
	}

	packetOut.write16(packetOut.length()-packetOut.position()-2);
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

	Logs::Dump(_packetOut,6,"Response:");

	if(symetric)
		RTMFP::Encode(_packetOut);
	else
		RTMFP::Encode(_aesEncrypt,_packetOut);

	RTMFP::Pack(_packetOut,_farId);

	try {
		// TODO remake? without retry (but flow)
		bool retry=false;
		while(_socket.sendTo(_packetOut.begin(),_packetOut.length(),_peer.address)!=_packetOut.length()) {
			if(retry) {
				ERROR("Socket send error on session '%u' : all data were not sent",_id);
				break;
			}
			retry = true;
		}
	} catch(Exception& ex) {
		 CRITIC("Socket send error on session '%u' : %s",_id,ex.displayText().c_str());
	}
}

void Session::packetHandler(PacketReader& packet) {

	_recvTimestamp.update();

	// Read packet
	UInt8 marker = packet.read8()|0xF0;
	
	_timeSent = packet.read16();

	// with time echo
	if(marker == 0xFD) {
		UInt16 ping = RTMFP::Time(_recvTimestamp.epochMicroseconds())-packet.read16();
		DEBUG("Ping : %hu",ping);
	} else if(marker != 0xF9)
		WARN("Packet marker unknown : %02x",marker);

	// Begin a possible response
	PacketWriter& packetOut = writer();

	// Variables for request (0x10 and 0x11)
	UInt8 flag;
	UInt8 idFlow=0;
	UInt8 stage;

	UInt8 type = packet.available()>0 ? packet.read8() : 0xFF;
	bool answer = false;

	// Can have nested queries
	while(type!=0xFF) {

		UInt16 size = packet.read16();
		UInt8 idResponse = 0;

		{
			PacketReader request(packet.current(),size);

			PacketWriter response(packetOut,3); // skip the futur possible id and length response

			switch(type) {
				case 0x0c :
					setFailed("Session failed on the client side");
					break;

				case 0x4c :
					/// Session death!
					kill();
					break;

				/// KeepAlive
				case 0x01 :
					idResponse = 0x41;
				case 0x41 :
					_timesKeepalive=0;
					break;

				// This following message is not really understood (flow header interrogation perhaps?)
				// So we emulate a mechanism which seems to answer the same thing when it's necessary
				case 0x5e : {
					UInt8 flowId = request.read8();
					idResponse = 0x10;
					response.write8(3);
					response.write8(flowId);
					response.write8(flow(flowId).stage());
					response.write8(0); // perhaps 0|1 for stage completed|not completed
					break;
				}

				case 0x18 : {
					/// This response is sent when we answer with a Acknowledgment negative
					// It contains the id flow
					// Resend the last Acknowledgment success
					// I don't unsertand the usefulness...
					UInt8 idFlow = request.read8();
					UInt8 stage = flow(idFlow).stage();
					if(stage>0) {
						idResponse = 0x51;
						response.write8(idFlow);
						response.write8(0x3f);
						response.write8(stage);
					}
					break;
				}
				case 0x51 : {
					/// Acknowledgment 
					UInt8 idFlow= request.read8();
					UInt8 ack = request.read8();
					while(ack==0xFF)
						ack = request.read8();
					if(ack>0) // is usually equal to 0x7f
						flow(idFlow).acknowledgment(request.read8());
					else
						ERROR("The flow '%02x' has received a ack negative for the stage '%02x'",idFlow,stage);
					// else {
					// In fact here, we should send a 0x18 message (with id flow),
					// but it can create a loop... We prefer cancel the message
					break;
				}
				/// Request
				// 0x10 normal request
				// 0x11 special request, in repeat case (following stage request)
				case 0x10 :
					flag = request.read8(); // Unknown, is 0x80 or 0x00
					idFlow= request.read8();
					stage = request.read8()-1;
				case 0x11 : {
					++stage;
					// It happens when the previus stage is not ack,
					// and the 6 next bytes are the 6 bytes of previus flow request in the same location.
					// Unknown why it's really used, but it can be omitted.
					// In this case, we must remove the 7 next bytes
					if(request.read8()==0x02)
						request.next(7);

					// Prepare response
					packetOut.next(3);
					packetOut.write8(flag);
					packetOut.write8(idFlow);
					packetOut.write8(stage);
					packetOut.write8(0x01); // Send always a 0x01 flag here means that we considerate that
					// the possible previus stage message is ack, even if it's wrong, because we can considerate
					// to keep things simple that a request is a acknowledgment for its response

					bool connecting = idFlow==FLOW_CONNECTION && stage==1;

					// Process request
					if(flow(idFlow,true).request(stage,request,packetOut)) {
						idResponse = 0x10;
						if(connecting)
							_clientAccepted = true;
					} else {
						packetOut.clear(response.position()-3);
						if(connecting && !_clientAccepted)
							setFailed("Client rejected");
					}

					break;
				}
				default :
					ERROR("Request type '%02x' unknown",type);
			}

			// No response for a failed session!
			if(_failed)
				idResponse=0;
			if(idResponse==0)
				response.clear();
		}
		
		if(idResponse>0) {
			answer = true;
			if(type != 0x10 && type != 0x11) {
				packetOut.write8(idResponse);
				int len = packetOut.length()-packetOut.position()-2;
				if(len<0)
					len = 0;
				packetOut.write16(len);
				packetOut.next(len);
			}
		}

		// Next
		packet.next(size);
		type = packet.available()>0 ? packet.read8() : 0xFF;

		// Write Acknowledgment
		if(idFlow>0 && type!= 0x11) {
			answer=true;
			packetOut.write8(0x51);
			packetOut.write16(3);
			packetOut.write8(idFlow);
			packetOut.write8(0x3f); // ack!
			packetOut.write8(stage);
			idFlow=0;
		}
	}

	if(answer)
		send();
}

// Don't must return a null value!
Flow* Session::createFlow(UInt8 id) {
	if(id==FLOW_CONNECTION) // NetConnection
		return new FlowConnection(_peer,_data,_pClientHandler);	
	else if(id>FLOW_CONNECTION) // NetGroup
		return new FlowGroup(_peer,_data);
	else if(id>0)
		ERROR("Flow id '%02x' unknown",id);	
	return new FlowNull(_peer,_data);
}




} // namespace Cumulus
