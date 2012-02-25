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

#include "FlowWriter.h"
#include "Util.h"
#include "Logs.h"
#include "string.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

MessageNull FlowWriter::_MessageNull;


FlowWriter::FlowWriter(const string& signature,BandWriter& band) : critical(false),id(0),_stage(0),_stageAck(0),_closed(false),_callbackHandle(0),_resetCount(0),flowId(0),_band(band),signature(signature),_repeatable(0),_lostCount(0),_ackCount(0) {
	band.initFlowWriter(*this);
}

FlowWriter::FlowWriter(FlowWriter& flowWriter) :
		id(flowWriter.id),critical(flowWriter.critical),
		_stage(flowWriter._stage),_stageAck(flowWriter._stageAck),
		_ackCount(flowWriter._ackCount),_lostCount(flowWriter._lostCount),
		_closed(false),_callbackHandle(0),_resetCount(0),
		flowId(0),_band(flowWriter._band),signature(flowWriter.signature) {
	close();
}

FlowWriter::~FlowWriter() {
	_closed=true;
	clear();
}


void FlowWriter::clear() {
	// delete messages
	Message* pMessage;
	while(!_messages.empty()) {
		pMessage = _messages.front();
		_lostCount += pMessage->fragments.size();
		delete pMessage;
		_messages.pop_front();
	}
	while(!_messagesSent.empty()) {
		pMessage = _messagesSent.front();
		_lostCount += pMessage->fragments.size();
		if(pMessage->repeatable)
			--_repeatable;
		delete pMessage;
		_messagesSent.pop_front();
	}
	if(_stage>0) {
		writeAbandonMessage(); // Send a MESSAGE_ABANDONMENT just in the case where the receiver has been created
		flush();
		_trigger.stop();
	}
}

void FlowWriter::fail(const string& error) {
	WARN("FlowWriter %u has failed : %s",id,error.c_str());
	_stage=_stageAck=_lostCount=_ackCount=0;
	clear();
	if(_closed)
		return;
	_band.resetFlowWriter(*new FlowWriter(*this));
	_band.initFlowWriter(*this);
	reset(++_resetCount);
}


void FlowWriter::close() {
	if(_closed)
		return;
	if(_stage>0 && _messages.size()==0)
		writeAbandonMessage(); // Send a MESSAGE_END just in the case where the receiver has been created
	_closed=true;
	flush();
}

void FlowWriter::acknowledgment(PacketReader& reader) {
	
	UInt32 bufferSize = reader.read7BitValue(); // TODO use this value in reliability mechanism?
	
	if(bufferSize==0) {
		// In fact here, we should send a 0x18 message (with id flow),
		// but it can create a loop... We prefer the following behavior
		fail("Negative acknowledgment");
		return;
	}

	UInt32 stageAckPrec = _stageAck;
	UInt32 stageReaden = reader.read7BitValue();
	UInt32 stage = _stageAck+1;

	if(stageReaden>_stage) {
		ERROR("Acknowledgment received %u superior than the current sending stage %u on flowWriter %u",stageReaden,_stage,id);
		_stageAck = _stage;
	} else if(stageReaden<=_stageAck) {
		// already acked
		if(reader.available()==0)
			DEBUG("Acknowledgment %u obsolete on flowWriter %u",stageReaden,id);
	} else
		_stageAck = stageReaden;

	UInt32 maxStageRecv = stageReaden;
	UInt32 pos=reader.position();

	while(reader.available()>0)
		maxStageRecv += reader.read7BitValue()+reader.read7BitValue()+2;
	if(pos != reader.position()) {
		// TRACE("%u..x%s",stageReaden,Util::FormatHex(reader.current(),reader.available()).c_str());
		reader.reset(pos);
	}

	UInt32 lostCount = 0,lostStage = 0;
	bool repeated = false;
	bool header = true;
	bool stop=false;

	list<Message*>::iterator it=_messagesSent.begin();
	while(!stop && it!=_messagesSent.end()) {
		Message& message(**it);

		if(message.fragments.empty()) {
			CRITIC("Message %u is bad formatted on fowWriter %u",stage+1,id);
			++it;
			continue;
		}

		map<UInt32,UInt32>::iterator itFrag=message.fragments.begin();
		while(message.fragments.end()!=itFrag) {
			
			// ACK
			if(_stageAck>=stage) {
				message.fragments.erase(message.fragments.begin());
				itFrag=message.fragments.begin();
				++_ackCount;
				++stage;
				continue;
			}

			// Read lost informations
			while(!stop) {
				if(lostCount==0) {
					if(reader.available()>0) {
						lostCount = reader.read7BitValue()+1;
						lostStage = stageReaden+1;
						stageReaden = lostStage+lostCount+reader.read7BitValue();
					} else {
						stop=true;
						break;
					}
				}
				// check the range
				if(lostStage>_stage) {
					// Not yet sent
					ERROR("Lost information received %u have not been yet sent on flowWriter %u",lostStage,id);
					stop=true;
				} else if(lostStage<=_stageAck) {
					// already acked
					--lostCount;
					++lostStage;
					continue;
				}
				break;
			}
			if(stop)
				break;
			
			// lostStage > 0 and lostCount > 0

			if(lostStage!=stage) {
				if(repeated) {
					++stage;
					++itFrag;
					header=true;
				} else // No repeated, it means that past lost packet was not repeatable, we can ack this intermediate received sequence
					_stageAck = stage;
				continue;
			}

			/// Repeat message asked!
			if(!message.repeatable) {
				if(repeated) {
					++itFrag;
					++stage;
					header=true;
				} else {
					INFO("FlowWriter %u : message %u lost",id,stage);
					--_ackCount;
					++_lostCount;
					_stageAck = stage;
				}
				--lostCount;
				++lostStage;
				continue;
			}

			repeated = true;
			// Don't repeate before that the receiver receives the itFrag->second sending stage
			if(itFrag->second >= maxStageRecv) {
				++stage;
				header=true;
				--lostCount;
				++lostStage;
				++itFrag;
				continue;
			}

			// Repeat message

			DEBUG("FlowWriter %u : stage %u repeated",id,stage);
			UInt32 available;
			UInt32 fragment(itFrag->first);
			BinaryReader& content = message.reader(fragment,available);
			itFrag->second = _stage; // Save actual stage sending to wait that the receiver gets it before to retry
			UInt32 contentSize = available;
			++itFrag;

			// Compute flags
			UInt8 flags = 0;
			if(fragment>0)
				flags |= MESSAGE_WITH_BEFOREPART; // fragmented
			if(itFrag!=message.fragments.end()) {
				flags |= MESSAGE_WITH_AFTERPART;
				contentSize = itFrag->first - fragment;
			}

			PacketWriter& packet(_band.writer());
			UInt32 size = contentSize+4;
			
			if(!header && size>packet.available()) {
				_band.flush(false);
				header=true;
			}
			
			if(header)
				size+=headerSize(stage);

			if(size>packet.available())
				_band.flush(false);

			// Write packet
			size-=3;  // type + timestamp removed, before the "writeMessage"
			flush(_band.writeMessage(header ? 0x10 : 0x11,(UInt16)size)
				,stage,flags,header,content,contentSize);
			available -= contentSize;
			header=false;
			--lostCount;
			++lostStage;
			++stage;
		}

		if(message.fragments.empty()) {
			if(message.repeatable)
				--_repeatable;
			if(_ackCount>0) {
				UInt32 available(0),size(0);
				BinaryReader& reader = message.memAck(available,size);
				ackMessageHandler(_ackCount,_lostCount,reader,available,size);
				_ackCount=_lostCount=0;
			}
			delete *it;
			it=_messagesSent.erase(it);
		} else
			++it;
	
	}

	if(lostCount>0 && reader.available()>0)
		ERROR("Some lost information received have not been yet sent on flowWriter %u",id);


	// rest messages repeatable?
	if(_repeatable==0)
		_trigger.stop();
	else if(_stageAck>stageAckPrec || repeated)
		_trigger.reset();
}

void FlowWriter::manage(Invoker& invoker) {
	if(consumed() || _band.failed())
		return;
	try {
		if(_trigger.raise())
			raiseMessage();
	} catch(...) {
		fail("FlowWriter can't deliver its data");
		throw;
	}
	if(critical && _closed)
		throw Exception("Critical flow writer closed, session must be closed");
	flush();
}

UInt32 FlowWriter::headerSize(UInt32 stage) {
	UInt32 size= Util::Get7BitValueSize(id);
	size+= Util::Get7BitValueSize(stage);
	if(_stageAck>stage)
		CRITIC("stageAck %u superior to stage %u on flowWriter %u",_stageAck,stage,id);
	size+= Util::Get7BitValueSize(stage-_stageAck);
	size+= _stageAck>0 ? 0 : (signature.size()+(flowId==0?2:(4+Util::Get7BitValueSize(flowId))));
	return size;
}


void FlowWriter::flush(PacketWriter& writer,UInt32 stage,UInt8 flags,bool header,BinaryReader& reader,UInt16 size) {
	if(_stageAck==0 && header)
		flags |= MESSAGE_HEADER;
	if(size==0)
		flags |= MESSAGE_ABANDONMENT;
	if(_closed)
		flags |= MESSAGE_END;

	// TRACE("FlowWriter %u stage %u",id,stage);

	writer.write8(flags);

	if(header) {
		writer.write7BitValue(id);
		writer.write7BitValue(stage);
		writer.write7BitValue((flags&MESSAGE_ABANDONMENT) ? 0 : (stage-_stageAck));

		// signature
		if(_stageAck==0) {
			writer.writeString8(signature);
			// No write this in the case where it's a new flow!
			if(flowId>0) {
				writer.write8(1+Util::Get7BitValueSize(flowId)); // following size
				writer.write8(0x0a); // Unknown!
				writer.write7BitValue(flowId);
			}
			writer.write8(0); // marker of end for this part
		}

	}

	if(size>0) {
		reader.readRaw(writer.begin()+writer.position(),size);
		writer.next(size);
	}
}

void FlowWriter::raiseMessage() {
	list<Message*>::const_iterator it;
	bool header = true;
	bool stop = true;
	bool sent = false;
	UInt32 stage = _stageAck+1;

	for(it=_messagesSent.begin();it!=_messagesSent.end();++it) {
		Message& message(**it);
		
		if(message.fragments.empty())
			break;

		// not repeat unbuffered messages
		if(!message.repeatable) {
			stage += message.fragments.size();
			header = true;
			continue;
		}
		
		/// HERE -> message repeatable AND already flushed one time!

		if(stop) {
			_band.flush(false); // To repeat message, before we must send precedent waiting mesages
			stop = false;
		}

		map<UInt32,UInt32>::const_iterator itFrag=message.fragments.begin();
		UInt32 available;
		BinaryReader& content = message.reader(available);
		
		while(itFrag!=message.fragments.end()) {
			UInt32 contentSize = available;
			UInt32 fragment(itFrag->first);
			++itFrag;

			// Compute flags
			UInt8 flags = 0;
			if(fragment>0)
				flags |= MESSAGE_WITH_BEFOREPART; // fragmented
			if(itFrag!=message.fragments.end()) {
				flags |= MESSAGE_WITH_AFTERPART;
				contentSize = itFrag->first - fragment;
			}

			PacketWriter& packet(_band.writer());
			UInt32 size = contentSize+4;

			if(header)
				size+=headerSize(stage);

			// Actual sending packet is enough large? Here we send just one packet!
			if(size>packet.available()) {
				if(!sent)
					ERROR("Raise messages on flowWriter %u without sending!",id);
				DEBUG("Raise message on flowWriter %u finishs on stage %u",id,stage);
				return;
			}
			sent=true;

			// Write packet
			size-=3;  // type + timestamp removed, before the "writeMessage"
			flush(_band.writeMessage(header ? 0x10 : 0x11,(UInt16)size)
				,stage++,flags,header,content,contentSize);
			available -= contentSize;
			header=false;
		}
	}

	if(stop)
		_trigger.stop();
}

void FlowWriter::flush(bool full) {

	if(_messagesSent.size()>100)
		DEBUG("_messagesSent.size()=%lu",_messagesSent.size());

	// flush
	bool header = !_band.canWriteFollowing(*this);

	list<Message*>::const_iterator it=_messages.begin();
	while(it!=_messages.end()) {
		Message& message(**it);
		if(message.repeatable) {
			++_repeatable;
			_trigger.start();
		}

		UInt32 fragments= 0;

		UInt32 available;
		BinaryReader& content = message.reader(available);

		do {

			PacketWriter& packet(_band.writer());

			// Actual sending packet is enough large?
			if(packet.available()<12) { // 12 to have a size minimum of fragmentation!
				_band.flush(false); // send packet (and without time echo)
				header=true;
			}

			bool head = header;

			UInt32 contentSize = available;
			UInt32 size = contentSize+4;
			++_stage;

			if(head)
				size+=headerSize(_stage);

			// Compute flags
			UInt8 flags = 0;
			if(fragments>0)
				flags |= MESSAGE_WITH_BEFOREPART;

			if(size>packet.available()) {
				// the packet will change! The message will be fragmented.
				flags |= MESSAGE_WITH_AFTERPART;
				contentSize = packet.available()-(size-contentSize);
				size=packet.available();
				header=true;
			} else
				header=false; // the packet stays the same!

			// Write packet
			size-=3; // type + timestamp removed, before the "writeMessage"
			flush(_band.writeMessage(head ? 0x10 : 0x11,(UInt16)size,this),_stage,flags,head,content,contentSize);

			message.fragments[fragments] = _stage;
			available -= contentSize;
			fragments += contentSize;

		} while(available>0);

		_messagesSent.push_back(&message);
		_messages.pop_front();
		it=_messages.begin();
	}

	if(full)
		_band.flush();
}

void FlowWriter::cancel(UInt32 index) {
	if(index>=queue()) {
		ERROR("Impossible to cancel %u message on flowWriter %u",index,id);
		return;
	}
	list<Message*>::iterator it = _messages.begin();
	advance(it,index);
	delete *it;
	_messages.erase(it);
}

void FlowWriter::writeUnbufferedMessage(const UInt8* data,UInt32 size,const UInt8* memAckData,UInt32 memAckSize) {
	if(_closed || signature.empty() || _band.failed()) // signature.empty() means that we are on the flowWriter of FlowNull
		return;
	MessageUnbuffered* pMessage = new MessageUnbuffered(data,size,memAckData,memAckSize);
	_messages.push_back(pMessage); 
	flush();
}

MessageBuffered& FlowWriter::createBufferedMessage() {
	if(_closed || signature.empty() || _band.failed()) // signature.empty() means that we are on the flowWriter of FlowNull
		return _MessageNull;
	MessageBuffered* pMessage = new MessageBuffered();
	_messages.push_back(pMessage);
	return *pMessage;
}
BinaryWriter& FlowWriter::writeRawMessage(bool withoutHeader) {
	MessageBuffered& message(createBufferedMessage());
	if(!withoutHeader) {
		message.rawWriter.write8(0x04);
		message.rawWriter.write32(0);
	}
	return message.rawWriter;
}
AMFWriter& FlowWriter::writeAMFPacket(const string& name) {
	MessageBuffered& message(createBufferedMessage());
	BinaryWriter& writer = message.rawWriter;
	writer.write8(Message::AMF);writer.write8(0);writer.write32(0);
	writer.write8(AMF_STRING);writer.writeString16(name);
	return message.amfWriter;
}

void FlowWriter::writeResponseHeader(BinaryWriter& writer,const string& name,double callbackHandle) {
	writer.write8(Message::AMF_WITH_HANDLER);writer.write32(0);
	writer.write8(AMF_STRING);writer.writeString16(name);
	writer.write8(AMF_NUMBER); // marker
	writer << callbackHandle;
	writer.write8(AMF_NULL);
}
AMFWriter& FlowWriter::writeAMFMessage(const std::string& name) {
	MessageBuffered& message(createBufferedMessage());
	writeResponseHeader(message.rawWriter,name,0);
	return message.amfWriter;
}

AMFWriter& FlowWriter::writeAMFResult() {
	MessageBuffered& message(createBufferedMessage());
	writeResponseHeader(message.rawWriter,"_result",_callbackHandle);
	return message.amfWriter;
}

AMFObjectWriter FlowWriter::writeAMFResponse(const string& name,const string& code,const string& description) {
	MessageBuffered& message(createBufferedMessage());
	writeResponseHeader(message.rawWriter,name,_callbackHandle);

	string entireCode(_obj);
	if(!code.empty()) {
		entireCode.append(".");
		entireCode.append(code);
	}
	
	bool precValue = message.amfWriter.amf0Preference;
	message.amfWriter.amf0Preference=true;
	AMFObjectWriter object(message.amfWriter);
	if(name=="_error")
		object.write("level","error");
	else
		object.write("level","status");
	object.write("code",entireCode);
	if(!description.empty())
		object.write("description",description);
	message.amfWriter.amf0Preference = precValue;
	return object;
}


} // namespace Cumulus
