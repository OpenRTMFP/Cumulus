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


class Packet {
public:
	Packet(PacketReader& fragment) : fragments(1),_pMessage(NULL),_buffer((string::size_type)fragment.available()) {
		if(_buffer.size()>0)
			memcpy(&_buffer[0],fragment.current(),_buffer.size());
	}
	~Packet() {
		if(_pMessage)
			delete _pMessage;
	}

	void add(PacketReader& fragment) {
		string::size_type old = _buffer.size();
		_buffer.resize(old + (string::size_type)fragment.available());
		if(_buffer.size()>old)
			memcpy(&_buffer[old],fragment.current(),(size_t)fragment.available());
		++(UInt16&)fragments;
	}

	PacketReader* release() {
		if(_pMessage) {
			ERROR("Packet already released!");
			return _pMessage;
		}
		_pMessage = new PacketReader(_buffer.size()==0 ? NULL : &_buffer[0],_buffer.size());
		(UInt16&)_pMessage->fragments = fragments;
		return _pMessage;
	}

	const UInt32	fragments;
private:
	vector<UInt8>	_buffer;
	PacketReader*  _pMessage;
};


class Fragment : public Buffer<UInt8> {
public:
	Fragment(PacketReader& data,UInt8 flags) : flags(flags),Buffer<UInt8>(data.available()) {
		data.readRaw(begin(),size());
	}
	Poco::UInt8					flags;
};


Flow::Flow(UInt32 id,const string& signature,const string& name,Peer& peer,Handler& handler,BandWriter& band) : id(id),stage(0),peer(peer),handler(handler),_completed(false),_name(name),_pPacket(NULL),_band(band),writer(*new FlowWriter(signature,band)) {
	if(writer.flowId==0)
		((UInt32&)writer.flowId)=id;
	// create code prefix for a possible response
	writer._obj.assign(_name);
}

Flow::~Flow() {
	complete(); // just to display "consumed"

	// delete fragments
	map<UInt32,Fragment*>::const_iterator it;
	for(it=_fragments.begin();it!=_fragments.end();++it)
		delete it->second;

	// delete receive buffer
	if(_pPacket)
		delete _pPacket;

	writer.close();
}

void Flow::complete() {
	if(!_completed && !writer.signature.empty())
		DEBUG("Flow %u consumed",id);
	_completed=true;
}

void Flow::fail(const string& error) {
	ERROR("Flow %u failed : %s",id,error.c_str());
	if(!_completed) {
		BinaryWriter& writer = _band.writeMessage(0x5e,Util::Get7BitValueSize(id)+1);
		writer.write7BitValue(id);
		writer.write8(0); // unknown
	}
}

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

void Flow::commit() {
	// ACK
	PacketWriter& ack = _band.writeMessage(0x51,1+Util::Get7BitValueSize(id)+Util::Get7BitValueSize(stage));
	ack.write7BitValue(id);
	ack.write8(writer.signature.empty() ? 0x00 : 0x7f);
	ack.write7BitValue(stage);

	commitHandler();
	writer.flush();
}

void Flow::fragmentHandler(UInt32 stage,UInt32 deltaNAck,PacketReader& fragment,UInt8 flags) {
	if(_completed)
		return;

	UInt32 nextStage = this->stage+1;

	if(stage < nextStage) {
		DEBUG("Stage %u on flow %u has already been received",stage,id);
		return;
	}

	if(deltaNAck>stage || deltaNAck==0)
		deltaNAck=stage;
	
	if(flags&MESSAGE_ABANDONMENT || this->stage < (stage-deltaNAck)) {
		DEBUG("Abandonment signal (flag:%2x)",flags);
		map<UInt32,Fragment*>::iterator it=_fragments.begin();
		while(it!=_fragments.end()) {
			if( it->first > stage) // Abandon all stages <= stage
				break;
			if( it->first <= (stage-1)) {
				PacketReader reader(it->second->begin(),it->second->size());
				fragmentSortedHandler(it->first,reader,it->second->flags);
			}
			delete it->second;
			_fragments.erase(it++);
		}

		nextStage = stage;
	}

	
	if(stage>nextStage) {
		// not following stage!
		if(_fragments.find(stage) == _fragments.end()) {
			_fragments[stage] = new Fragment(fragment,flags);
			 if(_fragments.size()>100)
				DEBUG("_fragments.size()=%d",_fragments.size()); 
		} else
			DEBUG("Stage %u on flow %u has already been received",stage,id);
	} else {
		fragmentSortedHandler(nextStage++,fragment,flags);
		map<UInt32,Fragment*>::iterator it=_fragments.begin();
		while(it!=_fragments.end()) {
			if( it->first >nextStage)
				break;
			PacketReader reader(it->second->begin(),it->second->size());
			fragmentSortedHandler(nextStage++,reader,it->second->flags);
			delete it->second;
			_fragments.erase(it++);
		}

	}
}

void Flow::fragmentSortedHandler(UInt32 stage,PacketReader& fragment,UInt8 flags) {
	if(stage<=this->stage) {
		ERROR("Stage %u not sorted on flow %u",stage,id);
		return;
	}
	if(stage>(this->stage+1)) {
		// not following stage!
		lostFragmentsHandler(stage-this->stage-1);
		(UInt32&)this->stage = stage;
		if(_pPacket) {
			delete _pPacket;
			_pPacket = NULL;
		}
		if(flags&MESSAGE_WITH_BEFOREPART)
			return;
	} else
		(UInt32&)this->stage = stage;
	
	PacketReader* pMessage(&fragment);
	if(flags&MESSAGE_WITH_BEFOREPART){
		if(!_pPacket) {
			WARN("A received message tells to have a 'beforepart' and nevertheless partbuffer is empty, certainly some packets were lost");
			delete _pPacket;
			_pPacket = NULL;
			return;
		}
		
		_pPacket->add(fragment);

		if(flags&MESSAGE_WITH_AFTERPART)
			return;

		pMessage = _pPacket->release();
	} else if(flags&MESSAGE_WITH_AFTERPART) {
		if(_pPacket) {
			ERROR("A received message tells to have not 'beforepart' and nevertheless partbuffer exists");
			delete _pPacket;
		}
		_pPacket = new Packet(fragment);
		return;
	}

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
	writer._callbackHandle = 0;

	if(_pPacket) {
		delete _pPacket;
		_pPacket=NULL;
	}

	if(flags&MESSAGE_END)
		complete();
}

void Flow::messageHandler(const std::string& name,AMFReader& message) {
	ERROR("Message '%s' unknown for flow %u",name.c_str(),id);
}
void Flow::rawHandler(UInt8 type,PacketReader& data) {
	ERROR("Raw message %s unknown for flow %u",Util::FormatHex(data.current(),data.available()).c_str(),id);
}
void Flow::audioHandler(PacketReader& packet) {
	ERROR("Audio packet untreated for flow %u",id);
}
void Flow::videoHandler(PacketReader& packet) {
	ERROR("Video packet untreated for flow %u",id);
}
void Flow::lostFragmentsHandler(UInt32 count) {
	INFO("%u fragments lost on flow %u",count,id);
}



} // namespace Cumulus
