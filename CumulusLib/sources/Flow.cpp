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
#include "Logs.h"
#include "Util.h"
#include "string.h"

#define EMPTY				0x00
#define AUDIO				0x08
#define VIDEO				0x09
#define AMF_WITH_HANDLER	0x14
#define AMF					0x0F

using namespace std;
using namespace Poco;

namespace Cumulus {


Flow::Flow(UInt32 id,const string& signature,const string& name,Peer& peer,ServerHandler& serverHandler,BandWriter& band) : id(id),_stage(0),peer(peer),serverHandler(serverHandler),_completed(false),_name(name),_signature(signature),_pBuffer(NULL),_sizeBuffer(0),_band(band),writer(*new FlowWriter(id,signature,band)) {
}

Flow::~Flow() {
	complete();
	// delete receive buffer
	if(_sizeBuffer>0) {
		delete [] _pBuffer;
		_sizeBuffer=0;
	}
}

void Flow::complete() {
	if(!_completed && id>0)
		DEBUG("Flow '%u' consumed",id);
	_completed=true;
}

void Flow::flush() {
	writer.flush();
	if(_completed)
		writer.close();
}

void Flow::fail(const string& error) {
	ERROR("Flow %u failed : %s",id,error.c_str());
	if(!_completed) {
		BinaryWriter& writer = _band.writeMessage(0x5e,Util::Get7BitValueSize(id)+1);
		writer.write7BitValue(id);
		writer.write8(0); // unknown
	}
}

/*
Flow& Flow::newFlow() {
	return _session.newFlow(_signature);
}*/

UInt8 Flow::unpack(PacketReader& reader) {
	if(reader.available()==0)
		return EMPTY;
	UInt8 type = reader.read8();
	switch(type) {
		// amf content
		case 0x11:
			reader.next(1);
		case AMF_WITH_HANDLER:
			reader.next(4);
			return AMF_WITH_HANDLER;
		case AMF:
			reader.next(5);
		case AUDIO:
		case VIDEO:
			break;
		// raw data
		case 0x04:
			reader.next(4);
		case 0x01:
			break;
		default:
			ERROR("Unpacking type '%02x' unknown",type);
			break;
	}
	return type;
}

void Flow::messageHandler(UInt32 stage,PacketReader& message,UInt8 flags) {
	if(_completed)
		return;

	if(stage<=_stage) {
		DEBUG("Stage %u on flow %u has already been received",stage,id);
		return;
	}
	_stage = stage;

	PacketReader* pMessage(NULL);
	if(flags&MESSAGE_WITH_BEFOREPART){
		if(_sizeBuffer==0) {
			ERROR("A received message tells to have a 'afterpart' and nevertheless partbuffer is empty");
			return;
		}
		
		UInt8* pOldBuffer = _pBuffer;
		_pBuffer = new UInt8[_sizeBuffer + message.available()]();
		memcpy(_pBuffer,pOldBuffer,_sizeBuffer);
		memcpy(_pBuffer+_sizeBuffer,message.current(),message.available());
		_sizeBuffer += message.available();
		delete [] pOldBuffer;

		if(flags&MESSAGE_WITH_AFTERPART)
			return;

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

	UInt8 type = unpack(*pMessage);
	if(type!=EMPTY) {
		writer._callbackHandle = 0;
		string name;
		AMFReader amf(*pMessage);
		if(type==AMF_WITH_HANDLER || type==AMF) {
			amf.read(name);
			if(type==AMF_WITH_HANDLER) {
				writer._callbackHandle = amf.readNumber();
				amf.skipNull();
			}
		}

		// create code prefix
		writer._code.assign(_name);
		if(!name.empty()) {
			writer._code.append(".");
			writer._code.push_back(toupper(name[0]));
			if(name.size()>1)
				writer._code.append(&name[1]);
		}

		switch(type) {
			case AMF_WITH_HANDLER:
			case AMF:
				messageHandler(name,amf);
				break;
			case AUDIO:
				audioHandler(*pMessage);
				break;
			case VIDEO:
				videoHandler(*pMessage);
				break;
			default:
				rawHandler(type,*pMessage);
		}
	}

	delete pMessage;

	if(flags&MESSAGE_END)
		complete();

	if(_sizeBuffer>0) {
		delete [] _pBuffer;
		_sizeBuffer=0;
	}
}

void Flow::messageHandler(const std::string& name,AMFReader& message) {
	ERROR("Message '%s' unknown for flow '%u'",name.c_str(),id);
}
void Flow::rawHandler(UInt8 type,PacketReader& data) {
	ERROR("Raw message unknown for flow '%u'",id);
}
void Flow::audioHandler(PacketReader& packet) {
	ERROR("Audio packet untreated for flow '%u'",id);
}
void Flow::videoHandler(PacketReader& packet) {
	ERROR("Video packet untreated for flow '%u'",id);
}



} // namespace Cumulus
