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
#include "FlowStream.h"
#include "Poco/URI.h"
#include "Poco/Format.h"
#include "string.h"

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
				 ServerHandler& serverHandler) : 
		_id(id),_farId(farId),_socket(socket),_testDecode(false),
		_aesDecrypt(decryptKey,AESEngine::DECRYPT),_aesEncrypt(encryptKey,AESEngine::ENCRYPT),_serverHandler(serverHandler),_peer(peer),_flowNull(_peer,*this,_serverHandler),_died(false),_failed(false),_timesFailed(0),_timeSent(0),_timesKeepalive(0),_writer(_buffer,sizeof(_buffer)) {
	_writer.next(11);
	_writer.limit(RTMFP_MAX_PACKET_LENGTH); // set normal limit
}


Session::~Session() {
	kill();
	if(_peer.state!=Client::NONE)
		WARN("onDisconnect client handler has not been called on the session '%u'",_id);
	// delete flows
	map<UInt8,Flow*>::const_iterator it;
	for(it=_flows.begin();it!=_flows.end();++it)
		delete it->second;
	_flows.clear();
}

bool Session::decode(PacketReader& packet,const SocketAddress& sender) {
	((SocketAddress&)_peer.address) = sender;
	bool result = RTMFP::Decode(_aesDecrypt,packet);
	if(result)
		_testDecode = true;
	return result;
}

void Session::kill() {
	if(_died)
		return;
	if(_peer.state!=Client::NONE) {
		((Client::ClientState&)_peer.state) = Client::NONE;
		_serverHandler.disconnection(_peer);
	}
	_died=true;
	_failed=true;
}

void Session::manage() {
	if(_died)
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
			DEBUG("Flow '%02x' consumed",it->first);
			delete it->second;
			_flows.erase(it++);
			continue;
		}

		try {
			flow.raise();
		} catch(const Exception& ex) {
			fail(ex.displayText());
			return;
		}
		++it;
	}
	flush();
}

void Session::keepAlive() {
	DEBUG("Keepalive server");
	if(_timesKeepalive==10) {
		setFailed("Timeout keepalive attempts");
		return;
	}
	++_timesKeepalive;
	writeMessage(0x01,0);
}

void Session::setFailed(const string& msg) {
	if(_failed)
		return;
	_failed=true;
	if(_peer.state!=Client::NONE)
		_serverHandler.failed(_peer,msg);
	// Set flows in consumed state
	map<UInt8,Flow*>::const_iterator it;
	for(it=_flows.begin();it!=_flows.end();++it)
		it->second->complete();
	// unsubscribe peer for its groups
	_peer.unsubscribeGroups();
	WARN("Session failed on the server side : %s",msg.c_str());
}

void Session::fail() {
	if(_died)
		WARN("Fail perhaps useless because session is already dead");
	if(!_failed) {
		WARN("Here flag failed should be put (with setFailed method), fail() method just allows the fail packet sending");
		_failed=true;
	}
	++_timesFailed;
	PacketWriter& writer = this->writer(); 
	writer.write8(0x0C);
	writer.write16(0);
	flush(WITHOUT_ECHO_TIME); // We send immediatly the fail message
	// After 6 mn we can considerated than the session is died!
	if(_timesFailed==10 || _recvTimestamp.isElapsed(360000000))
		kill();
}

void Session::p2pHandshake(const SocketAddress& address,const std::string& tag,Session* pSession) {

	DEBUG("Peer newcomer address send to peer '%u' connected",id());
	
	Address const* pAddress = NULL;
	UInt16 size = 0x37 + (address.host().family() == IPAddress::IPv6 ? 16 : 4);

	if(pSession) {
		map<string,UInt8>::iterator it =	_p2pHandshakeAttemps.find(tag);
		if(it==_p2pHandshakeAttemps.end()) {
			it = _p2pHandshakeAttemps.insert(pair<string,UInt8>(tag,0)).first;
			// If two clients are on the same lan, starts with private address
			if(memcmp(address.addr(),peer().address.addr(),address.length())==0 && pSession->peer().privateAddress.size()>0)
				it->second=1;
		}
		
		if(it->second>0) {
			pAddress = &pSession->peer().privateAddress[it->second-1];
			size +=  pAddress->host.size();
		}
		++it->second;
		if(it->second > pSession->peer().privateAddress.size())
			it->second=0;
	}
	
	PacketWriter& writer = writeMessage(0x0F,size);

	writer.write8(34);
	writer.write8(33);

	writer.write8(0x0F);
	writer.writeRaw(_peer.id,32);
	
	if(pAddress)
		writer.writeAddress(*pAddress,false);
	else
		writer.writeAddress(address,true);

	writer.writeRaw(tag);

	flush();
}

void Session::flush(UInt8 flags) {
	PacketWriter& packet(writer());
	if(packet.length()>=RTMFP_MIN_PACKET_SIZE) {

		packet.limit(); // no limit for sending!

		// After 30 sec, send packet without echo time
		bool timeEcho = flags&WITHOUT_ECHO_TIME ? false : !_recvTimestamp.isElapsed(30000000);

		UInt8 marker = flags&SYMETRIC_ENCODING ? 0x0b : 0x4a;
		if(timeEcho)
			marker+=4;
		else
			packet.clip(2);

		packet.reset(6);
		packet.write8(marker);
		packet.write16(RTMFP::TimeNow());
		if(timeEcho)
			packet.write16(_timeSent+RTMFP::Time(_recvTimestamp.elapsed()));

		Logs::Dump(packet,6,"Response:");

		if(flags&SYMETRIC_ENCODING)
			RTMFP::Encode(packet);
		else
			RTMFP::Encode(_aesEncrypt,packet);

		RTMFP::Pack(packet,_farId);

		try {
			// TODO remake? without retry (but flow)
			bool retry=false;
			while(_socket.sendTo(packet.begin(),packet.length(),_peer.address)!=packet.length()) {
				if(retry) {
					ERROR("Socket send error on session '%u' : all data were not sent",_id);
					break;
				}
				retry = true;
			}
		} catch(Exception& ex) {
			 CRITIC("Socket send error on session '%u' : %s",_id,ex.displayText().c_str());
		}
		
		if(!timeEcho)
			packet.clip(-2);

		packet.clear(11);
		packet.limit(RTMFP_MAX_PACKET_LENGTH); // reset the normal limit
	}
}

PacketWriter& Session::writer() {
	if(!_writer.good()) {
		WARN("Writing packet failed : the writer has certainly exceeded the size set");
		_writer.reset(11);
	}
	_writer.limit(RTMFP_MAX_PACKET_LENGTH);
	return _writer;
}

PacketWriter& Session::writeMessage(UInt8 type,UInt16 length) {
	PacketWriter& writer = this->writer();

	// No sending formated message for a failed session!
	if(_failed) {
		writer.clear();
		writer.limit(writer.position());
		return writer;
	}

	UInt16 size = length + 3; // for type and size

	if(size>writer.available())
		flush(WITHOUT_ECHO_TIME); // send packet (and without time echo)

	writer.limit(writer.position()+size);

	writer.write8(type);
	writer.write16(length);

	return writer;
}

void Session::packetHandler(PacketReader& packet) {

	_recvTimestamp.update();

	// Read packet
	UInt8 marker = packet.read8()|0xF0;
	
	_timeSent = packet.read16();

	// with time echo
	if(marker == 0xFD)
		_peer.setPing(RTMFP::Time(_recvTimestamp.epochMicroseconds())-packet.read16());
	else if(marker != 0xF9)
		WARN("Packet marker unknown : %02x",marker);


	// Variables for request (0x10 and 0x11)
	UInt8 flags;
	Flow* pFlow=NULL;
	UInt32 stage=0;

	UInt8 type = packet.available()>0 ? packet.read8() : 0xFF;
	bool answer = false;

	// Can have nested queries
	while(type!=0xFF) {

		UInt16 size = packet.read16();

		PacketReader message(packet.current(),size);		

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
				writeMessage(0x41,0);
			case 0x41 :
				_timesKeepalive=0;
				break;

			case 0x5e :
				// Flow exception!
				flow(message.read8()).fail();
				break;

			case 0x18 :
				/// This response is sent when we answer with a Acknowledgment negative
				// It contains the id flow
				// I don't unsertand the usefulness...
				//pFlow = &flow(message.read8());
				//stage = pFlow->stageSnd();
				// For the moment, we considerate it like a exception
				fail("Ack negative exception"); // send fail message immediatly
				break;

			case 0x51 : {
				/// Acknowledgment 
				UInt8 idFlow= message.read8();
				UInt8 ack = message.read8();
				while(ack==0xFF)
					ack = message.read8();
				UInt32 stage = message.read7BitValue();
				flow(idFlow).acknowledgment(stage);
				if(ack==0) {
					WARN("The flow '%02x' has received a negative ack for the stage '%u'",idFlow,stage);
					flow(idFlow).fail();
				}
				// else {
				// In fact here, we should send a 0x18 message (with id flow),
				// but it can create a loop... We prefer cancel the message
				break;
			}
			/// Request
			// 0x10 normal request
			// 0x11 special request, in repeat case (following stage request)
			case 0x10 : {
				flags = message.read8();
				UInt8 idFlow = message.read8();
				stage = message.read7BitValue()-1;
				UInt8 nbStageNAck = message.read8();

				// has Header?
				if(flags & MESSAGE_HEADER) {
					string signature;
					message.readString8(signature);

					// create flow just on a stage of 1 (here 0)
					if(stage==0)
						pFlow = createFlow(signature,idFlow);
					else if(!(flags&MESSAGE_WITH_BEFOREPART))
						DEBUG("Stage for Flow creation should be egal to 1, Maybe is a repeat or following packet, otherwise flow '%02x' is already consumed",idFlow);

					// Header part useless (correspondence between idflow sender/receiver, but here to make things simple, we choose the same id!)
					UInt8 length=message.read8();
					while(length>0 && message.available()) {
						message.next(length);
						length=message.read8();
					}
					if(length>0)
						ERROR("Bad header message part, finished before scheduled");
				}
				
				if(!pFlow)
					pFlow = &flow(idFlow);

				if(stage > pFlow->stageRcv()) {
					if(nbStageNAck>1) {
						// CASE stage superior than the current receiving stage
						// Here it means that a message with a stage superior to the current receiving stage has been sent
						// We must ignore this message for that the client resend the precedent packet, but "ack" the last packet received to try to accelerate the packet repetition!
						// In true we should save the packet for more later (TODO?), but it happens very rarely!
						WARN("A packet has been lost on the flow '%02x'",idFlow);
						PacketWriter& writer = writeMessage(0x51,2+Util::Get7BitValueSize(pFlow->stageRcv()));
						writer.write8(idFlow);
						writer.write8(0x3f); // ack
						writer.write7BitValue(pFlow->stageRcv());
						pFlow=NULL;
						break;
					}
					else
						ERROR("A flow '%02x' message with a '%u' stage more superior than one with the current stage '%u' has been received'",idFlow,stage,pFlow->stageRcv());
				}
			}	
			case 0x11 : {
				++stage;
				
				if(!pFlow)
					break; // see above at "CASE stage superior than the current receiving stage"

				// has Header?
				if(type==0x11)
					flags = message.read8();

				// Process request
				pFlow->messageHandler(stage,message,flags);
				if(_peer.state==Client::REJECTED && !failed())
					fail("Client rejected"); // send fail message immediatly

				break;
			}
			default :
				ERROR("Message type '%02x' unknown",type);
		}

		// Next
		packet.next(size);
		type = packet.available()>0 ? packet.read8() : 0xFF;

		// Write Acknowledgment
		if(pFlow && stage>0 && type!= 0x11) {
			PacketWriter& writer = writeMessage(0x51,2+Util::Get7BitValueSize(stage));
			writer.write8(pFlow->id);
			writer.write8(0x3f); // ack
			writer.write7BitValue(stage);
			pFlow->flushMessages();
			pFlow=NULL;
		}
	}

	flush();
}


Flow& Session::flow(Poco::UInt8 id) {
	map<UInt8,Flow*>::const_iterator it = _flows.find(id);
	if(it==_flows.end()) {
		((UInt8&)_flowNull.id) = id;
		return _flowNull;
	}
	return *it->second;
}

Flow* Session::createFlow(const string& signature,Poco::UInt8 id) {
	Flow* pFlow=NULL;
	map<UInt8,Flow*>::const_iterator it=_flows.find(id);
	if(it==_flows.end()) {
		if(signature==FlowConnection::s_signature)
			pFlow = new FlowConnection(id,_peer,*this,_serverHandler);	
		else if(signature==FlowGroup::s_signature)
			pFlow = new FlowGroup(id,_peer,*this,_serverHandler);
		else if(signature.compare(0,FlowStream::s_signature.length(),FlowStream::s_signature)==0)
			pFlow = new FlowStream(id,signature,_peer,*this,_serverHandler);
		else
			ERROR("New flow unknown : %s",Util::FormatHex((const UInt8*)signature.c_str(),signature.size()).c_str());
		if(pFlow)
			_flows[id] = pFlow;
	} else {
		pFlow = it->second;
		DEBUG("Flow '%02x' already created : %s",id,Util::FormatHex((const UInt8*)signature.c_str(),signature.size()).c_str());
	}
	return pFlow;
}




} // namespace Cumulus
