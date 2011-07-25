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
				 Handler& handler) : 
		_id(id),_farId(farId),_socket(socket),pTarget(NULL),checked(false),
		_aesDecrypt(decryptKey,AESEngine::DECRYPT),_aesEncrypt(encryptKey,AESEngine::ENCRYPT),_handler(handler),_peer(peer),_died(false),_failed(false),_timesFailed(0),_timeSent(0),_nextFlowWriterId(0),_timesKeepalive(0),_pLastFlowWriter(NULL),_writer(_buffer,sizeof(_buffer)) {
	_pFlowNull = new FlowNull(_peer,_handler,*this);
	_writer.next(11);
	_writer.limit(RTMFP_MAX_PACKET_LENGTH); // set normal limit
}


Session::~Session() {
	kill();
	if(_peer.state!=Peer::NONE)
		WARN("onDisconnect client handler has not been called on the session '%u'",_id);

	// delete flows
	map<UInt32,Flow*>::const_iterator it;
	for(it=_flows.begin();it!=_flows.end();++it)
		delete it->second;
	delete _pFlowNull;

	// delete flowWriters remaining
	map<UInt32,FlowWriter*>::const_iterator it2;
	for(it2=_flowWriters.begin();it2!=_flowWriters.end();++it2)
		delete it2->second;
	if(pTarget)
		delete pTarget;
}

bool Session::decode(PacketReader& packet,const SocketAddress& sender) {
	((SocketAddress&)_peer.address) = sender;
	if(pTarget)
		((SocketAddress&)pTarget->address) = sender;
	return RTMFP::Decode(_aesDecrypt,packet);
}

void Session::kill() {
	if(_died)
		return;
	if(_peer.state!=Peer::NONE) {
		((Peer::PeerState&)_peer.state) = Peer::NONE;
		_handler.onDisconnection(_peer);
		--((UInt32&)_handler.count);
	}
	_died=true;
	_failed=true;
}

void Session::manage() {
	if(_died)
		return;

	if(_failed) {
		failSignal();
		return;
	}

	// After 6 mn we considerate than the session has failed
	if(_recvTimestamp.isElapsed(360000000)) {
		fail("Timeout no client message");
		return;
	}

	// To accelerate the deletion of peer ghost (mainly for netgroup efficient), starts a keepalive server after 2 mn
	if(_recvTimestamp.isElapsed(120000000) && !keepAlive())
		return;

	// Raise FlowWriter
	map<UInt32,FlowWriter*>::iterator it2=_flowWriters.begin();
	while(it2!=_flowWriters.end()) {
		if(it2->second->consumed()) {
			delete it2->second;
			_flowWriters.erase(it2++);
			continue;
		}
		try {
			it2->second->manage(_handler);
		} catch(const Exception& ex) {
			if(it2->second->critical) {
				fail(ex.message()); // TODO no maybe good here. If a FlowWriter raise, we must kill the entiere session?
				return;
			}
			continue;
		}
		++it2;
	}
	flush();
}

bool Session::keepAlive() {
	DEBUG("Keepalive server");
	if(_timesKeepalive==10) {
		fail("Timeout keepalive attempts");
		return false;
	}
	++_timesKeepalive;
	writeMessage(0x01,0);
	return true;
}

void Session::fail(const string& error) {
	if(_failed)
		return;
	_failed=true;
	if(_peer.state!=Peer::NONE)
		_handler.onFailed(_peer,error);
	// close FlowWriters, here no new sending must happen except "failSignal"
	map<UInt32,FlowWriter*>::const_iterator it;
	for(it=_flowWriters.begin();it!=_flowWriters.end();++it)
		it->second->close();
	Session::writer().clear(11);
	// unsubscribe peer for its groups
	_peer.unsubscribeGroups();
	WARN("Session failed on the server side : %s",error.c_str());
	failSignal();
}

void Session::failSignal() {
	if(_died)
		WARN("Fail perhaps useless because session is already dead");
	if(!_failed) {
		WARN("Here flag failed should be put (with setFailed method), fail() method just allows the fail packet sending");
		_failed=true;
	}
	++_timesFailed;
	PacketWriter& writer = Session::writer(); 
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
	UInt16 size = 0x36;

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

	if(!pAddress)
		size += (address.host().family() == IPAddress::IPv6 ? 16 : 4);
	
	PacketWriter& writer = writeMessage(0x0F,size);

	writer.write8(0x22);
	writer.write8(0x21);

	writer.write8(0x0F);
	writer.writeRaw(_peer.id,ID_SIZE);
	
	if(pAddress)
		writer.writeAddress(*pAddress,false);
	else
		writer.writeAddress(address,true);

	writer.writeRaw(tag);

	flush();
}

void Session::flush(UInt8 flags) {
	_pLastFlowWriter=NULL;
	PacketWriter& packet(Session::writer());
	if(packet.length()>=RTMFP_MIN_PACKET_SIZE) {

		packet.limit(); // no limit for sending!

		// After 30 sec, send packet without echo time
		bool timeEcho = flags&WITHOUT_ECHO_TIME ? false : !_recvTimestamp.isElapsed(30000000);

		bool symetric = flags&SYMETRIC_ENCODING;
		UInt8 marker = symetric ? 0x0b : 0x4a;

		UInt32 offset=0;
		if(timeEcho)
			marker+=4;
		else
			offset = 2;

		{
			ScopedMemoryClip clip((MemoryStreamBuf&)*packet.stream().rdbuf(),offset);

			packet.reset(6);
			packet.write8(marker);
			packet.write16(RTMFP::TimeNow());
			if(timeEcho)
				packet.write16(_timeSent+RTMFP::Time(_recvTimestamp.elapsed()));

			Logs::Dump(packet,6,format("Response to %s",_peer.address.toString()).c_str());

			if(symetric)
				RTMFP::Encode(packet);
			else
				RTMFP::Encode(_aesEncrypt,packet);

			RTMFP::Pack(packet,_farId);

			try {
				// TODO remake? without retry (but flow)
				bool retry=false;
				while(_socket.sendTo(packet.begin(),(int)packet.length(),_peer.address)!=packet.length()) {
					if(retry) {
						ERROR("Socket send error on session '%u' : all data were not sent",_id);
						break;
					}
					retry = true;
				}
			} catch(Exception& ex) {
				 CRITIC("Socket send error on session '%u' : %s",_id,ex.message().c_str());
			}
		}

		packet.clear(11);
		packet.limit(RTMFP_MAX_PACKET_LENGTH); // reset the normal limit
	}
}

PacketWriter& Session::writer() {
	if(!_writer.good()) {
		if(!_failed)
			WARN("Writing packet failed : the writer has certainly exceeded the size set");
		_writer.reset(11);
	}
	_writer.limit(RTMFP_MAX_PACKET_LENGTH);
	return _writer;
}

PacketWriter& Session::writeMessage(UInt8 type,UInt16 length,FlowWriter* pFlowWriter) {
	PacketWriter& writer = Session::writer();

	// No sending formated message for a failed session!
	if(_failed) {
		writer.clear(11);
		writer.limit(writer.position());
		return writer;
	}

	_pLastFlowWriter=pFlowWriter;

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
	UInt32 deltaNAck=0;

	UInt8 type = packet.available()>0 ? packet.read8() : 0xFF;
	bool answer = false;

	// Can have nested queries
	while(type!=0xFF) {

		UInt16 size = packet.read16();

		PacketReader message(packet.current(),size);		

		switch(type) {
			case 0x0c :
				fail("Session failed on the client side");
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

			case 0x5e : {
				// Flow exception!
				UInt32 id = message.read7BitValue();
				
				FlowWriter* pFlowWriter = flowWriter(id);
				if(pFlowWriter)
					pFlowWriter->fail("receiver has rejected the flow");
				else
					WARN("FlowWriter %u unfound for failed signal",id);
				break;

			}
			case 0x18 :
				/// This response is sent when we answer with a Acknowledgment negative
				// It contains the id flow
				// I don't unsertand the usefulness...
				//pFlow = &flow(message.read8());
				//stage = pFlow->stageSnd();
				// For the moment, we considerate it like a exception
				fail("ack negative from server"); // send fail message immediatly
				break;

			case 0x51 : {
				/// Acknowledgment
				UInt32 id = message.read7BitValue();
				FlowWriter* pFlowWriter = flowWriter(id);
				if(pFlowWriter) {
					UInt8 ack = message.read8();
					while(ack==0xFF)
						ack = message.read8();
					if(ack>0)
						pFlowWriter->acknowledgment(message.read7BitValue());
					else {
						// In fact here, we should send a 0x18 message (with id flow),
						// but it can create a loop... We prefer the following behavior
						pFlowWriter->fail("ack negative from client");
					}

				} else
					WARN("FlowWriter %u unfound for acknowledgment",id);
				break;
			}
			/// Request
			// 0x10 normal request
			// 0x11 special request, in repeat case (following stage request)
			case 0x10 : {
				flags = message.read8();
				UInt32 idFlow = message.read7BitValue();
				stage = message.read7BitValue()-1;
				deltaNAck = message.read7BitValue()-1;

				map<UInt32,Flow*>::const_iterator it = _flows.find(idFlow);
				pFlow = it==_flows.end() ? NULL : it->second;

				// Header part if present
				if(flags & MESSAGE_HEADER) {
					string signature;
					message.readString8(signature);

					if(!pFlow)
						pFlow = createFlow(idFlow,signature);

					if(message.read8()>0) {

						// Fullduplex header part
						if(message.read8()!=0x0A)
							WARN("Unknown fullduplex header part for the flow '%u'",idFlow)
						else
							message.read7BitValue(); // Fullduplex useless here! Because we are creating a new Flow!

						// Useless header part 
						UInt8 length=message.read8();
						while(length>0 && message.available()) {
							WARN("Unknown message part on flow '%u'",idFlow);
							message.next(length);
							length=message.read8();
						}
						if(length>0)
							ERROR("Bad header message part, finished before scheduled");
					}

				}
				
				if(!pFlow) {
					WARN("Flow %u unfound",idFlow);
					((UInt32&)_pFlowNull->id) = idFlow;
					pFlow = _pFlowNull;
				}

			}	
			case 0x11 : {
				++stage;
				++deltaNAck;

				// has Header?
				if(type==0x11)
					flags = message.read8();

				// Process request
				pFlow->fragmentHandler(stage,deltaNAck,message,flags);
				if(!pFlow->error().empty())
					fail(pFlow->error()); // send fail message immediatly

				break;
			}
			default :
				ERROR("Message type '%02x' unknown",type);
		}

		// Next
		packet.next(size);
		type = packet.available()>0 ? packet.read8() : 0xFF;

		// Commit Flow
		if(pFlow && stage>0 && type!= 0x11) {
			pFlow->commit();
			if(pFlow->consumed()) {
				_flows.erase(pFlow->id);
				delete pFlow;
			}
			pFlow=NULL;
		}
	}

	flush();
}

FlowWriter* Session::flowWriter(Poco::UInt32 id) {
	map<UInt32,FlowWriter*>::const_iterator it = _flowWriters.find(id);
	if(it==_flowWriters.end())
		return NULL;
	return it->second;
}

Flow& Session::flow(Poco::UInt32 id) {
	map<UInt32,Flow*>::const_iterator it = _flows.find(id);
	if(it==_flows.end()) {
		WARN("Flow %u unfound",id);
		((UInt32&)_pFlowNull->id) = id;
		return *_pFlowNull;
	}
	return *it->second;
}

Flow* Session::createFlow(UInt32 id,const string& signature) {
	map<UInt32,Flow*>::const_iterator it = _flows.find(id);
	if(it!=_flows.end()) {
		WARN("Flow %u has already been created",id);
		return it->second;
	}

	Flow* pFlow=NULL;

	if(signature==FlowConnection::Signature)
		pFlow = new FlowConnection(id,_peer,_handler,*this);	
	else if(signature==FlowGroup::Signature)
		pFlow = new FlowGroup(id,_peer,_handler,*this);
	else if(signature.compare(0,FlowStream::Signature.length(),FlowStream::Signature)==0)
		pFlow = new FlowStream(id,signature,_peer,_handler,*this);
	else
		ERROR("New unknown flow '%s' on session %u",Util::FormatHex((const UInt8*)signature.c_str(),signature.size()).c_str(),this->id());
	if(pFlow) {
		DEBUG("New flow %u on session %u",id,this->id());
		_flows[id] = pFlow;
	}
	return pFlow;
}

void Session::initFlowWriter(FlowWriter& flowWriter) {
	while(++_nextFlowWriterId==0 || _flowWriters.find(_nextFlowWriterId)!=_flowWriters.end());
	(UInt32&)flowWriter.id = _nextFlowWriterId;
	if(_flows.begin()!=_flows.end())
		(UInt32&)flowWriter.flowId = _flows.begin()->second->id;
	_flowWriters[_nextFlowWriterId] = &flowWriter;
}

void Session::resetFlowWriter(FlowWriter& flowWriter) {
	_flowWriters[flowWriter.id] = &flowWriter;
}



} // namespace Cumulus
