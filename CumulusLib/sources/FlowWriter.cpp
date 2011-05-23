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


FlowWriter::FlowWriter(UInt32 flowId,const string& signature,BandWriter& band) : id(0),flowId(flowId),_stage(0),_closed(false),_band(band),signature(signature),_callbackHandle(0) {
	_messageNull.rawWriter.stream().setstate(ios_base::eofbit);
	band.initFlowWriter(*this);
}

FlowWriter::~FlowWriter() {
	// delete messages
	list<Message*>::const_iterator it;
	for(it=_messages.begin();it!=_messages.end();++it)
		delete *it;
}

void FlowWriter::close() {
	if(_closed)
		return;
	if(_stage>0)
		createMessage(); // Send a MESSAG_END just in the case where the receiver has been created
	_closed=true; // before the flush messages to set the MESSAG_END flag
	flush();
}

void FlowWriter::acknowledgment(Poco::UInt32 stage) {
	if(stage>_stage) {
		ERROR("Acknowledgment received superior than the current sending stage : '%u' instead of '%u'",stage,_stage);
		return;
	}
	list<Message*>::const_iterator it=_messages.begin();
	if(_messages.empty() || stage<=(*it)->startStage) {
		DEBUG("Acknowledgment of stage '%u' received lower than all repeating messages of flow '%u', certainly a obsolete ack packet",stage,id);
		return;
	}
	
	// Ack!
	// Here repeating messages exist, and minStage < stage <=_stageSnd
	UInt32 count = stage - (*it)->startStage;

	while(count>0 && it!=_messages.end() && !(*it)->fragments.empty()) { // if _fragments.empty(), it's a message not sending yet (not flushed)
		Message& message(**it);
		
		while(count > 0 && !message.fragments.empty()) {
			message.fragments.pop_front();
			--count;
			++message.startStage;
		}

		if(message.fragments.empty()) {
			delete *it;
			_messages.pop_front();
			it=_messages.begin();
		}
	}

	// rest messages not ack?
	if(_messages.begin()!=_messages.end() && !(*_messages.begin())->fragments.empty())
		_trigger.reset();
	else
		_trigger.stop();
}

void FlowWriter::raise() {
	if(!_trigger.raise())
		return;
	raiseMessage();
}

void FlowWriter::raiseMessage() {
	if(_messages.empty())
		_trigger.stop();

	list<Message*>::const_iterator it;
	bool header = true;
	UInt8 nbStageNAck=0;

	for(it=_messages.begin();it!=_messages.end();++it) {
		Message& message(**it);

		// just messages not flushed, so nothing to do
		if(message.fragments.empty())
			return;

		UInt32 stage = message.startStage;

		list<UInt32>::const_iterator itFrag=message.fragments.begin();
		UInt32 fragment(*itFrag);
		bool end=false;
		message.reset();
		
		while(!end && message.fragments.end()!=itFrag++) {
			streamsize size = message.available();
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
			Int8 signatureLen = (stage-nbStageNAck)>0 ? 0 : (signature.size()+(flowId==0?2:(4+flowIdSize)));
			if(header) {
				size+=idSize;
				size+=stageSize;
				size+=1; // nbStageNAck
				size+=signatureLen;
			}

			// Actual sending packet is enough large? Here we send just one packet!
			if(!header && size>packet.available())
				return;
			
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
			PacketWriter& writer = _band.writeMessage(header ? 0x10 : 0x11,size);
			
			size-=1;
			writer.write8(flags);
			
			if(header) {
				writer.write7BitValue(id);
				size-=idSize;
				writer.write7BitValue(++stage);
				size-=stageSize;
				writer.write8(++nbStageNAck);
				size-=1;
				
				// signature
				if((stage-nbStageNAck)==0) {
					writer.writeString8(signature);
					size -= (signature.size()+1);
					// No write this in the case where it's a new flow!
					if(flowId>0) {
						writer.write8(1+flowIdSize); // following size
						writer.write8(0x0a); // Unknown!
						size -= 2;
						writer.write8(flowId);
						size -= flowIdSize;
					}
					writer.write8(0); // marker of end for this part
					size -= 1;
				}
			}

			message.read(writer,size);
			header=false;
		}
	}
}

void FlowWriter::flush() {
	list<Message*>::const_iterator it;
	bool header = true;
	UInt8 nbStageNAck=0;

	for(it=_messages.begin();it!=_messages.end();++it) {
		Message& message(**it);
		if(!message.fragments.empty()) {
			nbStageNAck += message.fragments.size();
			continue;
		}

		_trigger.start();

		message.startStage = _stage;

		UInt32 fragments= 0;

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
			Int8 signatureLen = (_stage-nbStageNAck)>0 ? 0 : (signature.size()+(flowId==0?2:(4+flowIdSize)));
			bool head = header;
			int size = message.available()+4;

			if(head) {
				size+=idSize;
				size+=stageSize;
				size+=1; // nbStageNAck
				size+=signatureLen;
			}

			// Compute flags
			UInt8 flags = _stage==0 ? MESSAGE_HEADER : 0x00;
			if(_closed)
				flags |= MESSAGE_END;
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
			PacketWriter& writer = _band.writeMessage(head ? 0x10 : 0x11,size);

			size-=1;
			writer.write8(flags);

			++_stage;

			if(head) {
				writer.write7BitValue(id);
				size-=idSize;
				writer.write7BitValue(_stage);
				size-=stageSize;
				writer.write8(++nbStageNAck);
				size-=1;

				// signature
				if((_stage-nbStageNAck)==0) {
					writer.writeString8(signature);
					size -= (signature.size()+1);
					// No write this in the case where it's a new flow!
					if(flowId>0) {
						writer.write8(1+flowIdSize); // following size
						writer.write8(0x0a); // Unknown!
						size -= 2;
						writer.write8(flowId);
						size -= flowIdSize;
					}
					writer.write8(0); // marker of end for this part
					size -= 1;
				}

			}

			
			message.read(writer,size);
			message.fragments.push_back(fragments);
			fragments += size;

		} while(message.available()>0);
	}
}

Message& FlowWriter::createMessage() {
	if(_closed || id==0)
		return _messageNull;
	Message* pMessage = new Message();
	_messages.push_back(pMessage);
	return *pMessage;
}
BinaryWriter& FlowWriter::writeRawMessage(bool withoutHeader) {
	Message& message(createMessage());
	if(!withoutHeader) {
		message.rawWriter.write8(0x04);
		message.rawWriter.write32(0);
	}
	return message.rawWriter;
}
AMFWriter& FlowWriter::writeAMFMessage(const std::string& name) {
	Message& message(createMessage());
	message.amfWriter.writeResponseHeader(name,_callbackHandle);
	return message.amfWriter;
}
AMFObjectWriter FlowWriter::writeSuccessResponse(const string& description,const string& name) {
	AMFWriter& message(writeAMFResult());

	string code(_code);
	if(!name.empty()) {
		code.append(".");
		code.append(name);
	}

	AMFObjectWriter object(message);
	object.write("level","status");
	object.write("code",code);
	if(!description.empty())
		object.write("description",description);
	return object;
}
AMFObjectWriter FlowWriter::writeStatusResponse(const string& name,const string& description) {
	Message& message(createMessage());
	message.amfWriter.writeResponseHeader("onStatus",_callbackHandle);

	string code(_code);
	if(!name.empty()) {
		code.append(".");
		code.append(name);
	}

	AMFObjectWriter object(message.amfWriter);
	object.write("level","status");
	object.write("code",code);
	if(!description.empty())
		object.write("description",description);
	return object;
}
AMFObjectWriter FlowWriter::writeErrorResponse(const string& description,const string& name) {
	Message& message(createMessage());
	message.amfWriter.writeResponseHeader("_error",_callbackHandle);

	string code(_code);
	if(!name.empty()) {
		code.append(".");
		code.append(name);
	}

	AMFObjectWriter object(message.amfWriter);
	object.write("level","error");
	object.write("code",code);
	if(!description.empty())
		object.write("description",description);

	WARN("'%s' response error : %s",code.c_str(),description.c_str());
	return object;
}



} // namespace Cumulus
