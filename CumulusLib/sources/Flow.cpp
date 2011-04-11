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

#include "Flow.h"
#include "Util.h"
#include "Logs.h"
#include "string.h"
#include "Session.h"

using namespace std;
using namespace Poco;

namespace Cumulus {


Flow::Flow(UInt8 id,const string& signature,const string& name,Peer& peer,Session& session,const ServerHandler& serverHandler) : id(id),_stageRcv(0),_stageSnd(0),peer(peer),serverHandler(serverHandler),_completed(false),_name(name),_signature(signature),_pBuffer(NULL),_sizeBuffer(0),_callbackHandle(0),_session(session) {
	_messageNull._stream.setstate(ios_base::eofbit);
}

Flow::~Flow() {
	if(!_completed)
		complete();
	// delete messages
	list<MessageWriter*>::const_iterator it;
	for(it=_messages.begin();it!=_messages.end();++it)
		delete *it;
	// delete receive buffer
	if(_sizeBuffer>0) {
		delete [] _pBuffer;
		_sizeBuffer=0;
	}
}

void Flow::acknowledgment(Poco::UInt32 stage) {
	if(stage>_stageSnd) {
		ERROR("Acknowledgment received superior than the current sending stage : '%u' instead of '%u'",stage,_stageSnd);
		return;
	}
	list<MessageWriter*>::const_iterator it=_messages.begin();
	if(_messages.empty() || stage<=(*it)->_startStage) {
		WARN("Acknowledgment of stage '%u' received lower than all repeating messages of flow '%02x', certainly a obsolete ack packet",stage,id);
		return;
	}
	
	// Ack!
	// Here repeating messages exist, and minStage < stage <=_stageSnd
	UInt32 count = stage - (*it)->_startStage;

	while(count>0 && it!=_messages.end() && !(*it)->_fragments.empty()) { // if _fragments.empty(), it's a message not sending yet (not flushed)
		MessageWriter& message(**it);
		
		while(count > 0 && !message._fragments.empty()) {
			message._fragments.pop_front();
			--count;
			++message._startStage;
		}

		if(message._fragments.empty()) {
			delete *it;
			_messages.pop_front();
			it=_messages.begin();
		}
	}

	// rest messages not ack?
	if(_messages.begin()!=_messages.end() && !(*_messages.begin())->_fragments.empty())
		_trigger.reset();
	else
		_trigger.stop();
}

bool Flow::unpack(PacketReader& reader) {
	if(reader.available()==0)
		return true;
	UInt8 flag = reader.read8();
	switch(flag) {
		// amf content
		case 0x11:
			reader.next(1);
		case 0x14:
			reader.next(4);
			return false;
		// raw data
		default:
			ERROR("Unpacking flag '%02x' unknown",flag);
			reader.reset(reader.position()-1);
			break;
		case 0x04:
			reader.next(4);
		case 0x01:
			;
	}
	return true;
}

void Flow::raise() {
	if(!_trigger.raise())
		return;
	raiseMessage();
}

void Flow::flush() {
	flushMessages();
	_session.flush();
}

void Flow::raiseMessage() {
	if(_messages.empty())
		_trigger.stop();

	list<MessageWriter*>::const_iterator it;
	bool header = true;
	UInt8 nbStageNAck=0;

	for(it=_messages.begin();it!=_messages.end();++it) {
		MessageWriter& message(**it);

		// just messages not flushed, so nothing to do
		if(message._fragments.empty())
			return;

		UInt32 stage = message._startStage;

		list<UInt32>::const_iterator itFrag=message._fragments.begin();
		UInt32 fragment(*itFrag);
		bool end=false;
		message._stream.resetReading(fragment);
		
		while(!end && message._fragments.end()!=itFrag++) {
			int size = message._stream.size();
			end = itFrag==message._fragments.end();
			if(!end) {
				size = (*itFrag)-fragment;
				fragment = *itFrag;
			}

			PacketWriter& packet(_session.writer());
			size+=4;

			UInt8 stageSize = Util::Get7BitValueSize(stage+1);
			if(header) {
				size+=2;
				size+=stageSize;
			}

			// Actual sending packet is enough large? Here we send just one packet!
			if(!header && size>packet.available())
				return;
			
			// Compute flags
			UInt8 flags = stage==0 ? MESSAGE_HEADER : 0x00;
			if(_completed)
				flags |= MESSAGE_END;
			if(stage > message._startStage)
				flags |= MESSAGE_WITH_BEFOREPART; // fragmented
			if(!end)
				flags |= MESSAGE_WITH_AFTERPART;


			// Write packet
			size-=3;
			PacketWriter& writer = _session.writeMessage(header ? 0x10 : 0x11,size);
			size-=1;
			
			writer.write8(flags);
			if(header) {
				writer.write8(id);
				writer.write7BitValue(++stage);
				writer.write8(++nbStageNAck);
				size-=2;
				size-=stageSize;
			}

			message.read(writer,size);
			header=false;
		}
	}
}

void Flow::flushMessages() {
	list<MessageWriter*>::const_iterator it;
	bool header = true;
	UInt8 nbStageNAck=0;

	for(it=_messages.begin();it!=_messages.end();++it) {
		MessageWriter& message(**it);
		if(!message._fragments.empty()) {
			nbStageNAck += message._fragments.size();
			continue;
		}

		_trigger.start();

		message._startStage = _stageSnd;

		UInt32 fragments= 0;

		do {

			PacketWriter& packet(_session.writer());

			// Actual sending packet is enough large?
			if(packet.available()<12) { // 12 to have a size minimum of fragmentation!
				_session.flush(WITHOUT_ECHO_TIME); // send packet (and without time echo)
				header=true;
			}

			bool head = header;
			int size = message._stream.size()+4;
			UInt8 stageSize = Util::Get7BitValueSize(_stageSnd+1);

			if(head) {
				size+=2;
				size+=stageSize;
			}

			// Compute flags
			UInt8 flags = _stageSnd==0 ? MESSAGE_HEADER : 0x00;
			if(_completed)
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
			size-=3;

			// Write packet
			PacketWriter& writer = _session.writeMessage(head ? 0x10 : 0x11,size);

			size-=1;
			
			writer.write8(flags);
			++_stageSnd;

			if(head) {
				writer.write8(id);
				writer.write7BitValue(_stageSnd);
				writer.write8(++nbStageNAck);
				size-=2;
				size-=stageSize;
			}
			
			message.read(writer,size);
			message._fragments.push_back(fragments);
			fragments += size;

		} while(message._stream.size()>0);
	}
}

void Flow::fail() {
	// This following message is not really understood (flow header interrogation perhaps?)
	// So we emulate a mechanism which seems to answer the same thing when it's necessary
	WARN("The flow '%02x' has failed",id);
	if(_completed)
		return;
	createMessage();
	complete(); // before the flush messages to set '_completed' to true
	flushMessages();
}

MessageWriter& Flow::createMessage() {
	if(_completed)
		return _messageNull;
	MessageWriter* pMessage = new MessageWriter();
	if(_stageSnd==0 && _messages.empty()) {
		pMessage->writeString8(_signature);
		pMessage->write8(0x02); // following size
		pMessage->write8(0x0a); // Unknown!
		pMessage->write8(id);
		pMessage->write8(0); // marker of end for this part
	}
	_messages.push_back(pMessage);
	return *pMessage;
}
MessageWriter& Flow::writeRawMessage(bool withoutHeader) {
	if(withoutHeader)
		return createMessage();
	MessageWriter& message(createMessage());
	message.write8(0x04);
	message.write32(0);
	return message;
}
AMFWriter& Flow::writeAMFMessage() {
	MessageWriter& message(createMessage());
	message.amf.writeResponseHeader("_result",_callbackHandle);
	return message.amf;
}
AMFObjectWriter Flow::writeSuccessResponse(const string& description,const string& name) {
	AMFWriter& message(writeAMFMessage());

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
AMFObjectWriter Flow::writeStatusResponse(const string& name,const string& description) {
	MessageWriter& message(createMessage());
	message.amf.writeResponseHeader("onStatus",_callbackHandle);

	string code(_code);
	if(!name.empty()) {
		code.append(".");
		code.append(name);
	}

	AMFObjectWriter object(message.amf);
	object.write("level","status");
	object.write("code",code);
	if(!description.empty())
		object.write("description",description);
	return object;
}
AMFObjectWriter Flow::writeErrorResponse(const string& description,const string& name) {
	MessageWriter& message(createMessage());
	message.amf.writeResponseHeader("_error",_callbackHandle);

	string code(_code);
	if(!name.empty()) {
		code.append(".");
		code.append(name);
	}

	AMFObjectWriter object(message.amf);
	object.write("level","error");
	object.write("code",code);
	if(!description.empty())
		object.write("description",description);

	WARN("'%s' response error : %s",code.c_str(),description.c_str());
	return object;
}

void Flow::messageHandler(UInt32 stage,PacketReader& message,UInt8 flags) {
	if(stage<=_stageRcv) {
		DEBUG("Flow '%02x' stage '%u' has already been received",id,stage);
		return;
	}
	_stageRcv = stage;

	PacketReader* pMessage(NULL);
	if(flags&MESSAGE_WITH_BEFOREPART){
		if(_sizeBuffer==0) {
			ERROR("A received message tells to have a 'afterpart' and nevertheless partbuffer is empty");
			return;
		}
		if(flags&MESSAGE_WITH_AFTERPART) {
			UInt8* pOldBuffer = _pBuffer;
			_pBuffer = new UInt8[_sizeBuffer + message.available()]();
			memcpy(_pBuffer,pOldBuffer,_sizeBuffer);
			memcpy(_pBuffer+_sizeBuffer,message.current(),message.available());
			_sizeBuffer += message.available();
			delete [] pOldBuffer;
			return;
		}
		pMessage = new PacketReader(_pBuffer,_sizeBuffer);
	} else if(flags&MESSAGE_WITH_AFTERPART) {
		if(_sizeBuffer>0) {
			ERROR("A received message tells to have not 'beforepart' and nevertheless partbuffer exists");
			delete [] _pBuffer;
			_sizeBuffer=0;
		}
		_sizeBuffer = message.available();
		_pBuffer = new UInt8[_sizeBuffer]();
		memcpy(_pBuffer,message.current(),_sizeBuffer);
		return;
	}
	if(!pMessage)
		pMessage = new PacketReader(message);

	bool isRaw = unpack(*pMessage);
	_callbackHandle = 0;
	string name;
	AMFReader reader(*pMessage);
	if(!isRaw) {
		reader.read(name);
		_callbackHandle = reader.readNumber();
		reader.skipNull();
	}

	// create code prefix
	_code.assign(_name);
	if(!name.empty()) {
		_code.append(".");
		_code.push_back(toupper(name[0]));
		if(name.size()>1)
			_code.append(&name[1]);
	}
	
	if(isRaw)
		rawHandler(*pMessage);
	else
		messageHandler(name,reader);
	delete pMessage;

	if(flags&MESSAGE_END)
		complete();

	if(_sizeBuffer>0) {
		delete [] _pBuffer;
		_sizeBuffer=0;
	}
}

void Flow::messageHandler(const std::string& name,AMFReader& message) {
	ERROR("Message '%s' unknown for flow '%02x'",name.c_str(),id);
}
void Flow::rawHandler(PacketReader& data) {
	ERROR("Raw message unknown for flow '%02x'",id);
}



} // namespace Cumulus
