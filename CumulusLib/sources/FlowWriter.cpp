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

#define EMPTY				0x00
#define AUDIO				0x08
#define VIDEO				0x09
#define AMF_WITH_HANDLER	0x14
#define AMF					0x0F

using namespace std;
using namespace Poco;

namespace Cumulus {


FlowWriter::FlowWriter(const string& signature,BandWriter& band) : critical(false),id(0),_stage(0),_closed(false),_callbackHandle(0),_resetCount(0),FlowWriterFactory(signature,band),_lostMessages(0) {
	_messageNull.rawWriter.stream().setstate(ios_base::eofbit);
	band.initFlowWriter(*this);
}

FlowWriter::FlowWriter(FlowWriter& flowWriter) :
		id(flowWriter.id),critical(flowWriter.critical),
		_stage(flowWriter._stage),_lostMessages(0),
		_closed(false),_callbackHandle(0),_resetCount(0),
		FlowWriterFactory(flowWriter.signature,flowWriter._band) {
	_messageNull.rawWriter.stream().setstate(ios_base::eofbit);
	close();
}

FlowWriter::~FlowWriter() {
	clearMessages();
}

void FlowWriter::clearMessages(bool exceptLast) {
	// delete messages
	UInt32 count = exceptLast ? 1 : 0;
	while(_messages.size()>count) {
		delete _messages.front();
		_messages.pop_front();
		++_lostMessages;
	}
	if(_messages.size()==0)
		_trigger.stop();
}

void FlowWriter::fail(const string& error) {
	WARN("FlowWriter %u has failed : %s",id,error.c_str());
	clearMessages();
	_band.resetFlowWriter(*new FlowWriter(*this));
	_band.initFlowWriter(*this);
	_stage=0;
	reset(++_resetCount);
}


void FlowWriter::close() {
	if(_closed)
		return;
	clearMessages(true);
	if(_stage>0 && count()==0)
		createBufferedMessage(); // Send a MESSAG_END | MESSAGE_ABANDONMENT just in the case where the receiver has been created
	_closed=true; // before the flush messages to set the MESSAG_END flag
	flush();
}

void FlowWriter::acknowledgment(UInt32 stage) {

	if(stage>_stage) {
		ERROR("Acknowledgment received superior than the current sending stage : %u instead of %u",stage,_stage);
		return;
	}

	list<Message*>::const_iterator it=_messages.begin();
	bool hasNAck = !_messages.empty() && !(*it)->fragments.empty();

	UInt32 startStage = hasNAck ? (*it)->startStage : (stage+1);
	Int64 count = stage - startStage;

	if(count==0) // Case where the ACK has been repeated, and is just below the last NAck message, certainly a disorder UDP transfer in packet received
		return;
	
	if(count<0) {
		DEBUG("Acknowledgment of stage '%u' received lower than all NACK messages of flow '%u', certainly a obsolete ack packet",stage,id);
		// TODO add a MESSAGE_ABANDONMENT agile system? it can meaning here too an error : a ACK volontary (and emphatically) send which is lower than all repeating messages = PB!
		return;
	}
	
	// Here minStage < stage <=_stageSnd

	// Ack!
	while(count>0 && it!=_messages.end() && !(*it)->fragments.empty()) { // if _fragments.empty(), it's a message not sending yet (not flushed)
		Message& message(**it);
		
		while(count > 0 && !message.fragments.empty()) {
			// ACK
			message.fragments.pop_front();
			--count;
			++message.startStage;
		}

		if(message.fragments.empty()) {
			UInt32 size(0);
			ackMessageHandler(message.memAck(size),size,_lostMessages);
			_lostMessages=0;
			delete *it;
			_messages.pop_front();
			it=_messages.begin();
		}
	}

	// rest messages not ack?
	if(!_messages.empty() && !(*_messages.begin())->fragments.empty())
		_trigger.reset();
	else
		_trigger.stop();
}

void FlowWriter::manage(Handler& handler) {
	try {
		if(!_trigger.raise())
			return;
	} catch(...) {
		clearMessages();
		throw;
	}
	raiseMessage();
}

void FlowWriter::raiseMessage() {
	if(_messages.empty() || (*_messages.begin())->fragments.empty()) {
		_trigger.stop();
		return;
	}

	_band.flush(WITHOUT_ECHO_TIME); // To repeat message, before we must send precedent waiting mesages

	list<Message*>::const_iterator it=_messages.begin();
	bool header = true;
	UInt32 deltaNAck=0;

	while(it!=_messages.end()) {
		Message& message(**it);

		// clear unbuffered messages
		if(!message.repeatable) {
			if(it==_messages.begin()) {
				_messages.pop_front();
				it=_messages.begin();
				++_lostMessages;
				if(_messages.empty())
					_trigger.stop();
			} else {
				deltaNAck += message.fragments.size();
				++it;
			}
			continue;
		}

		UInt32 stage = message.startStage;

		list<UInt32>::const_iterator itFrag=message.fragments.begin();
		UInt32 fragment(*itFrag);
		bool end=false;
		UInt32 available;
		BinaryReader& reader = message.reader(available);
		
		while(!end && message.fragments.end()!=itFrag++) {
			UInt32 size = available;
			end = itFrag==message.fragments.end();
			if(!end) {
				size = (*itFrag)-fragment;
				fragment = *itFrag;
			}

			PacketWriter& packet(_band.writer());
			size+=4;

			UInt8 stageSize = Util::Get7BitValueSize(stage+1);
			UInt8 idSize = Util::Get7BitValueSize(id);
			UInt8 flowIdSize = Util::Get7BitValueSize(flowId);
			UInt8 deltaNAckSize = Util::Get7BitValueSize(deltaNAck);
			UInt8 signatureLen = (stage-deltaNAck)>0 ? 0 : (signature.size()+(flowId==0?2:(4+flowIdSize)));
			if(header) {
				size+=idSize;
				size+=stageSize;
				size+=deltaNAckSize;
				size+=signatureLen;
			}

			// Actual sending packet is enough large? Here we send just one packet!
			if(!header && size>packet.available()) {
				DEBUG("Raise message on flowWriter %u finishs on stage %u",id,stage);
				return;
			}
			
			// Compute flags
			UInt8 flags = stage==0 ? MESSAGE_HEADER : 0x00;
			if(_closed)
				flags |= MESSAGE_END;
			if(stage > message.startStage)
				flags |= MESSAGE_WITH_BEFOREPART; // fragmented
			if(!end)
				flags |= MESSAGE_WITH_AFTERPART;


			// Write packet
			size-=3;  // type + timestamp removed, before the "writeMessage"
			PacketWriter& writer = _band.writeMessage(header ? 0x10 : 0x11,(UInt16)size,this);
			
			size-=1;
			writer.write8(flags);
			
			if(header) {
				writer.write7BitValue(id);
				size-=idSize;
				writer.write7BitValue(++stage);
				size-=stageSize;
				writer.write7BitValue(++deltaNAck);
				size-=deltaNAckSize;
				
				// signature
				if((stage-deltaNAck)==0) {
					writer.writeString8(signature);
					size -= (signature.size()+1);
					// No write this in the case where it's a new flow!
					if(flowId>0) {
						writer.write8(1+flowIdSize); // following size
						writer.write8(0x0a); // Unknown!
						size -= 2;
						writer.write7BitValue(flowId);
						size -= flowIdSize;
					}
					writer.write8(0); // marker of end for this part
					size -= 1;
				}
			}
			
			available -= size;
			reader.readRaw(writer.begin()+writer.position(),size);
			writer.next(size);

			header=false;
		}
		++it;
	}
}

void FlowWriter::flush(bool full) {
	list<Message*>::const_iterator it;
	bool header = !_band.canWriteFollowing(*this);
	UInt8 deltaNAck=0;

	for(it=_messages.begin();it!=_messages.end();++it) {
		Message& message(**it);
		if(!message.fragments.empty()) {
			deltaNAck += message.fragments.size();
			continue;
		}

		_trigger.start();

		message.startStage = _stage;

		UInt32 fragments= 0;

		UInt32 available;
		BinaryReader& reader = message.reader(available);

		do {

			PacketWriter& packet(_band.writer());

			// Actual sending packet is enough large?
			if(packet.available()<12) { // 12 to have a size minimum of fragmentation!
				_band.flush(WITHOUT_ECHO_TIME); // send packet (and without time echo)
				header=true;
			}

			UInt8 stageSize = Util::Get7BitValueSize(_stage+1);
			UInt8 idSize = Util::Get7BitValueSize(id);
			UInt8 flowIdSize = Util::Get7BitValueSize(flowId);
			UInt8 deltaNAckSize = Util::Get7BitValueSize(deltaNAck);
			UInt8 signatureLen = (_stage-deltaNAck)>0 ? 0 : (signature.size()+(flowId==0?2:(4+flowIdSize)));
			bool head = header;

			UInt32 size = available+4;

			if(head) {
				size+=idSize;
				size+=stageSize;
				size+=deltaNAckSize;
				size+=signatureLen;
			}

			// Compute flags
			UInt8 flags = _stage==0 ? MESSAGE_HEADER : 0x00;
			if(_closed)
				flags |= MESSAGE_END | MESSAGE_ABANDONMENT;
			if(fragments>0)
				flags |= MESSAGE_WITH_BEFOREPART;

			if(size>packet.available()) {
				// the packet will changed! The message will be fragmented.
				flags |= MESSAGE_WITH_AFTERPART;
				size=packet.available();
				header=true;
			} else
				header=false; // the packet stays the same!
			size-=3; // type + timestamp removed, before the "writeMessage"

			// Write packet
			PacketWriter& writer = _band.writeMessage(head ? 0x10 : 0x11,(UInt16)size,this);

			size-=1;
			writer.write8(flags);

			++_stage;

			if(head) {
				writer.write7BitValue(id);
				size-=idSize;
				writer.write7BitValue(_stage);
				size-=stageSize;
				writer.write7BitValue(++deltaNAck);
				size-=deltaNAckSize;

				// signature
				if((_stage-deltaNAck)==0) {
					writer.writeString8(signature);
					size -= (signature.size()+1);
					// No write this in the case where it's a new flow!
					if(flowId>0) {
						writer.write8(1+flowIdSize); // following size
						writer.write8(0x0a); // Unknown!
						size -= 2;
						writer.write7BitValue(flowId);
						size -= flowIdSize;
					}
					writer.write8(0); // marker of end for this part
					size -= 1;
				}

			}

			available -= size;
			reader.readRaw(writer.begin()+writer.position(),size);
			writer.next(size);

			message.fragments.push_back(fragments);
			fragments += size;

		} while(available>0);

	}
	if(full)
		_band.flush();
}

void FlowWriter::writeUnbufferedMessage(const UInt8* data,UInt32 size,const UInt8* memAckData,UInt32 memAckSize) {
	if(_closed || signature.empty()) // signature.empty() means that we are on the flowWriter of FlowNull
		return;
	MessageUnbuffered* pMessage = new MessageUnbuffered(data,size,memAckData,memAckSize);
	_messages.push_back(pMessage);
	 if(_messages.size()>100)
		DEBUG("_messages.size()=%d",_messages.size()); 
	flush();
}

MessageBuffered& FlowWriter::createBufferedMessage() {
	if(_closed || signature.empty()) // signature.empty() means that we are on the flowWriter of FlowNull
		return _messageNull;
	MessageBuffered* pMessage = new MessageBuffered();
	_messages.push_back(pMessage);
	 if(_messages.size()>100)
		DEBUG("_messages.size()=%d",_messages.size()); 
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
AMFWriter& FlowWriter::writeAMFMessage(const std::string& name) {
	MessageBuffered& message(createBufferedMessage());
	message.amfWriter.writeResponseHeader(name,_callbackHandle);
	return message.amfWriter;
}

AMFObjectWriter FlowWriter::writeAMFResponse(const string& name,const string& code,const string& description) {
	MessageBuffered& message(createBufferedMessage());
	message.amfWriter.writeResponseHeader(name,_callbackHandle);

	string entireCode(_obj);
	if(!code.empty()) {
		entireCode.append(".");
		entireCode.append(code);
	}

	AMFObjectWriter object(message.amfWriter);
	if(name=="_error") {
		object.write("level","error");
		
	} else
		object.write("level","status");
	object.write("code",entireCode);
	if(!description.empty())
		object.write("description",description);
	return object;
}


} // namespace Cumulus
