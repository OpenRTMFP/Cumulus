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

#include "ServerSession.h"
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

ServerSession::ServerSession(UInt32 id,
				 UInt32 farId,
				 const Peer& peer,
				 const UInt8* decryptKey,
				 const UInt8* encryptKey,
				 Invoker& invoker) : Session(id,farId,peer,decryptKey,encryptKey),pTarget(NULL),_invoker(invoker),_failed(false),_timesFailed(0),_timeSent(0),_nextFlowWriterId(0),_timesKeepalive(0),_pLastFlowWriter(NULL),_writer(_buffer,sizeof(_buffer)) {
	_pFlowNull = new FlowNull(this->peer,invoker,*this);
	_writer.next(11);
	_writer.limit(RTMFP_MAX_PACKET_LENGTH); // set normal limit
	
}


ServerSession::~ServerSession() {
	if(!died) {
		failSignal();
		kill();
	}
	// delete helloAttempts
	map<string,Attempt*>::const_iterator it0;
	for(it0=_helloAttempts.begin();it0!=_helloAttempts.end();++it0)
		delete it0->second;
	if(pTarget)
		delete pTarget;
}

void ServerSession::fail(const string& error) {
	if(_failed)
		return;

	// Here no new sending must happen except "failSignal"
	ServerSession::writer().clear(11);
	map<UInt64,FlowWriter*>::const_iterator it;
	for(it=_flowWriters.begin();it!=_flowWriters.end();++it)
		it->second->clear();
	peer.setFlowWriter(NULL);

	// unsubscribe peer for its groups
	peer.unsubscribeGroups();

	_failed=true;
	if(!error.empty()) {
		WARN("Session failed %u : %s",id,error.c_str());
		peer.onFailed(error);
		failSignal();
	}
	
}

void ServerSession::failSignal() {
	if(died)
		return;
	++_timesFailed;
	PacketWriter& writer = ServerSession::writer(); 
	writer.write8(0x0C);
	writer.write16(0);
	flush(false); // We send immediatly the fail message
	// After 6 mn we can considerated than the session is died!
	if(_timesFailed==10 || _recvTimestamp.isElapsed(360000000))
		kill();
}

void ServerSession::kill() {
	_failed=true;
	if(died)
		return;

	peer.setFlowWriter(NULL);

	// unsubscribe peer for its groups
	peer.unsubscribeGroups();

	// delete flows
	map<UInt64,Flow*>::const_iterator it1;
	for(it1=_flows.begin();it1!=_flows.end();++it1)
		delete it1->second;
	_flows.clear();
	delete _pFlowNull;
	
	peer.onDisconnection();

	// delete flowWriters
	map<UInt64,FlowWriter*>::const_iterator it2;
	for(it2=_flowWriters.begin();it2!=_flowWriters.end();++it2)
		delete it2->second;
	_flowWriters.clear();

	(bool&)died=true;
}

void ServerSession::manage() {
	if(died)
		return;

	// clean obsolete helloAttempts
	map<string,Attempt*>::iterator it=_helloAttempts.begin();
	while(it!=_helloAttempts.end()) {
		if(it->second->obsolete()) {
			delete it->second;
			_helloAttempts.erase(it++);
		} else
			++it;
	}

	if(_failed) {
		failSignal();
		return;
	} else if(peer.closed())
		fail("");

	// After 6 mn we considerate than the session has failed
	if(_recvTimestamp.isElapsed(360000000)) {
		fail("Timeout no client message");
		return;
	}

	// To accelerate the deletion of peer ghost (mainly for netgroup efficient), starts a keepalive server after 2 mn
	if(_recvTimestamp.isElapsed(120000000) && !keepAlive())
		return;

	// Raise FlowWriter
	map<UInt64,FlowWriter*>::iterator it2=_flowWriters.begin();
	while(it2!=_flowWriters.end()) {
		try {
			it2->second->manage(_invoker);
		} catch(const Exception& ex) {
			if(it2->second->critical) {
				fail(ex.message());
				break;
			}
			continue;
		}
		if(it2->second->consumed()) {
			if(it2->second->critical)
				fail("Critical flow writer closed, session must be closed");
			delete it2->second;
			_flowWriters.erase(it2++);
			continue;
		}
		++it2;
	}
	flush();
}

bool ServerSession::keepAlive() {
	if(!peer.connected) {
		fail("Timeout connection client");
		return false;
	}
	DEBUG("Keepalive server");
	if(_timesKeepalive==10) {
		fail("Timeout keepalive attempts");
		return false;
	}
	++_timesKeepalive;
	writeMessage(0x01,0);
	return true;
}

void ServerSession::eraseHelloAttempt(const string& tag) {
// clean obsolete helloAttempts
	map<string,Attempt*>::iterator it=_helloAttempts.find(tag);
	if(it==_helloAttempts.end()) {
		WARN("Hello attempt %s unfound, deletion useless",tag.c_str());
		return;
	}
	delete it->second;
	_helloAttempts.erase(it);
}


void ServerSession::p2pHandshake(const SocketAddress& address,const std::string& tag,Session* pSession) {
	if(_failed || peer.closed()) {
		if(!_failed)
			fail("");
		return;
	}

	bool good=true;

	if(pSession) {
		if(pSession->peer.addresses.size()==0) {
			CRITIC("Session %u without peer addresses!",pSession->id);
			good = false;
		}
	} else
		good = false;

	DEBUG("Peer newcomer address send to peer %u connected",id);
	
	UInt16 size = 0x36;
	UInt32 count = helloAttempt(tag);
	UInt8 index=0;

	Address const* pAddress = NULL;
	if(good) {
		// If two clients are on the same lan, starts with private address
		if(peer.addresses.size()==0)
			CRITIC("Session %u without peer addresses!",id)
		else if(pSession->peer.addresses.front().host==peer.addresses.front().host)
			++count;

		index=count%pSession->peer.addresses.size();
		list<Address>::const_iterator it=pSession->peer.addresses.begin();
		advance(it,index);
		pAddress = &(*it);
		size +=  pAddress->host.size();
	} else
		size += (address.host().family() == IPAddress::IPv6 ? 16 : 4);


	PacketWriter& writer = writeMessage(0x0F,size);

	writer.write8(0x22);
	writer.write8(0x21);

	writer.write8(0x0F);
	writer.writeRaw(peer.id,ID_SIZE);
	
	if(pAddress)
		writer.writeAddress(*pAddress,index==0);
	else
		writer.writeAddress(address,true);

	writer.writeRaw(tag);

	flush();
}

void ServerSession::flush(UInt8 marker,bool echoTime) {
	_pLastFlowWriter=NULL;
	if(died)
		return;

	PacketWriter& packet(ServerSession::writer());
	if(packet.length()>=RTMFP_MIN_PACKET_SIZE) {
		
		packet.limit(); // no limit for sending!

		// After 30 sec, send packet without echo time
		if(_recvTimestamp.isElapsed(30000000))
			echoTime = false;

		UInt32 offset=0;
		if(echoTime)
			marker+=4;
		else
			offset = 2;

		{
			ScopedMemoryClip clip((MemoryStreamBuf&)*packet.stream().rdbuf(),offset);

			packet.reset(6);
			packet.write8(marker);
			packet.write16(RTMFP::TimeNow());
			if(echoTime)
				packet.write16(_timeSent+RTMFP::Time(_recvTimestamp.elapsed()));
			
			Session::send(packet);
		}

		packet.clear(11);
		packet.limit(RTMFP_MAX_PACKET_LENGTH); // reset the normal limit
	}
}

PacketWriter& ServerSession::writer() {
	if(!_writer.good()) {
		if(!_failed)
			WARN("Writing packet failed : the writer has certainly exceeded the size set");
		_writer.reset(11);
	}
	_writer.limit(RTMFP_MAX_PACKET_LENGTH);
	return _writer;
}

PacketWriter& ServerSession::writeMessage(UInt8 type,UInt16 length,FlowWriter* pFlowWriter) {
	PacketWriter& writer = ServerSession::writer();

	// No sending formated message for a failed session!
	if(_failed) {
		writer.clear(11);
		writer.limit(writer.position());
		return writer;
	}

	_pLastFlowWriter=pFlowWriter;

	UInt16 size = length + 3; // for type and size

	if(size>writer.available()) {
		flush(false); // send packet (and without time echo)
		if(size > writer.available()) {
			CRITIC("Message truncated because exceeds maximum UDP packet size on session %u",id);
			size = writer.available();
		}
		_pLastFlowWriter=NULL;
	}

	writer.limit(writer.position()+size);

	writer.write8(type);
	writer.write16(length);

	return writer;
}

void ServerSession::packetHandler(PacketReader& packet) {
	if(died)
		return;

	if(!_failed && peer.closed())
		fail("");

	if(peer.addresses.size()==0) {
		CRITIC("Session %u has no any addresses!",id);
		peer.addresses.push_front(peer.address.toString());
	} else if(!(flags&SESSION_BY_EDGE) && peer.addresses.front()!=peer.address) {
		INFO("Session %u has changed its public address",id);
		peer.addresses.pop_front();
		peer.addresses.push_front(peer.address.toString());
	}

	_recvTimestamp.update();

	// Read packet
	UInt8 marker = packet.read8()|0xF0;
	
	_timeSent = packet.read16();

	// with time echo
	if(marker == 0xFD) {
		UInt16 time = RTMFP::TimeNow();
		UInt16 timeEcho = packet.read16();
		if(timeEcho>time) {
			if(timeEcho-time<30)
				time=0;
			else
				time += 0xFFFF-timeEcho;
			timeEcho = 0;
		}
		(UInt16&)peer.ping = (time-timeEcho)*RTMFP_TIMESTAMP_SCALE;
	}
	else if(marker != 0xF9)
		WARN("Packet marker unknown : %02x",marker);


	// Variables for request (0x10 and 0x11)
	UInt8 flags;
	Flow* pFlow=NULL;
	UInt64 stage=0;
	UInt64 deltaNAck=0;

	UInt8 type = packet.available()>0 ? packet.read8() : 0xFF;
	bool answer = false;

	// Can have nested queries
	while(type!=0xFF) {

		UInt16 size = packet.read16();

		PacketReader message(packet.current(),size);		

		switch(type) {
			case 0x70 : {
				// only for edge: tell that peer addess has changed!
				INFO("Edge tells that Session %u has changed its public address",id);
				string address;
				message >> address;
				if(peer.addresses.size()==0)
					CRITIC("Session %u has no any addresses!",id)
				else
					peer.addresses.pop_front();
				peer.addresses.push_front(address);
				writeMessage(0x71,address.size()).writeRaw(address);
				break;
			}
			case 0x0c :
				fail("failed on client side");
				break;

			case 0x4c :
				/// Session death!
				kill();
				return;

			/// KeepAlive
			case 0x01 :
				if(!peer.connected)
					fail("Timeout connection client");
				else
					writeMessage(0x41,0);
			case 0x41 :
				_timesKeepalive=0;
				break;

			case 0x5e : {
				// Flow exception!
				UInt64 id = message.read7BitLongValue();
				
				FlowWriter* pFlowWriter = flowWriter(id);
				if(pFlowWriter)
					pFlowWriter->fail(format("flowWriter rejected on session %u",this->id));
				else
					WARN("FlowWriter %llu unfound for failed signal on session %u",id,this->id);
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
				/*if(this->id>1) {
					vector<UInt8> out;
					Util::Dump(message.current(),message.available(),out);
					cout.write((const char*)&out[0],out.size());
				}*/
				UInt64 id = message.read7BitLongValue();
				FlowWriter* pFlowWriter = flowWriter(id);
				if(pFlowWriter)
					pFlowWriter->acknowledgment(message);
				else
					WARN("FlowWriter %llu unfound for acknowledgment on session %u",id,this->id);
				break;
			}
			/// Request
			// 0x10 normal request
			// 0x11 special request, in repeat case (following stage request)
			case 0x10 : {
				flags = message.read8();
				UInt64 idFlow = message.read7BitLongValue();
				stage = message.read7BitLongValue()-1;
				deltaNAck = message.read7BitLongValue()-1;
				
				if(_failed)
					break;

				map<UInt64,Flow*>::const_iterator it = _flows.find(idFlow);
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
							WARN("Unknown fullduplex header part for the flow %llu",idFlow)
						else
							message.read7BitLongValue(); // Fullduplex useless here! Because we are creating a new Flow!

						// Useless header part 
						UInt8 length=message.read8();
						while(length>0 && message.available()) {
							WARN("Unknown message part on flow %llu",idFlow);
							message.next(length);
							length=message.read8();
						}
						if(length>0)
							ERROR("Bad header message part, finished before scheduled");
					}

				}
				
				if(!pFlow) {
					WARN("Flow %llu unfound",idFlow);
					((UInt64&)_pFlowNull->id) = idFlow;
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
				if(pFlow) {
					pFlow->fragmentHandler(stage,deltaNAck,message,flags);
					if(!pFlow->error().empty() || peer.closed()) {
						fail(pFlow->error()); // send fail message immediatly
						pFlow = NULL;
					}
				}

				break;
			}
			default :
				ERROR("Message type '%02x' unknown",type);
		}

		// Next
		packet.next(size);
		type = packet.available()>0 ? packet.read8() : 0xFF;

		// Commit Flow
		if(pFlow && type!= 0x11) {
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

FlowWriter* ServerSession::flowWriter(Poco::UInt64 id) {
	map<UInt64,FlowWriter*>::const_iterator it = _flowWriters.find(id);
	if(it==_flowWriters.end())
		return NULL;
	return it->second;
}

Flow& ServerSession::flow(Poco::UInt64 id) {
	map<UInt64,Flow*>::const_iterator it = _flows.find(id);
	if(it==_flows.end()) {
		WARN("Flow %llu unfound",id);
		((UInt64&)_pFlowNull->id) = id;
		return *_pFlowNull;
	}
	return *it->second;
}

Flow* ServerSession::createFlow(UInt64 id,const string& signature) {
	if(died) {
		ERROR("Session %u is died, no more Flow creation possible",this->id);
		return NULL;
	}
	map<UInt64,Flow*>::const_iterator it = _flows.find(id);
	if(it!=_flows.end()) {
		WARN("Flow %llu has already been created",id);
		return it->second;
	}

	Flow* pFlow=NULL;

	if(signature==FlowConnection::Signature)
		pFlow = new FlowConnection(id,peer,_invoker,*this);	
	else if(signature==FlowGroup::Signature)
		pFlow = new FlowGroup(id,peer,_invoker,*this);
	else if(signature.compare(0,FlowStream::Signature.length(),FlowStream::Signature)==0)
		pFlow = new FlowStream(id,signature,peer,_invoker,*this);
	else
		ERROR("New unknown flow '%s' on session %u",Util::FormatHex((const UInt8*)signature.c_str(),signature.size()).c_str(),this->id);
	if(pFlow) {
		DEBUG("New flow %llu on session %u",id,this->id);
		_flows[id] = pFlow;
	}
	return pFlow;
}

void ServerSession::initFlowWriter(FlowWriter& flowWriter) {
	while(++_nextFlowWriterId==0 || _flowWriters.find(_nextFlowWriterId)!=_flowWriters.end());
	(UInt64&)flowWriter.id = _nextFlowWriterId;
	if(_flows.begin()!=_flows.end())
		(UInt64&)flowWriter.flowId = _flows.begin()->second->id;
	_flowWriters[_nextFlowWriterId] = &flowWriter;
}

void ServerSession::resetFlowWriter(FlowWriter& flowWriter) {
	_flowWriters[flowWriter.id] = &flowWriter;
}



} // namespace Cumulus
