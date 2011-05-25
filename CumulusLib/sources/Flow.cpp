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

class Fragment : public Buffer<UInt8> {
public:
	Fragment(PacketReader& data,UInt8 flags) : flags(flags),Buffer<UInt8>(data.available()) {
		data.readRaw(begin(),size());
	}
	Poco::UInt8					flags;
};


Flow::Flow(UInt32 id,const string& signature,const string& name,Peer& peer,ServerHandler& serverHandler,BandWriter& band) : id(id),stage(0),peer(peer),serverHandler(serverHandler),_completed(false),_name(name),_pBuffer(NULL),_band(band),writer(*new FlowWriter(id,signature,band)) {
	writer._bound=true;
}

Flow::~Flow() {
	complete();

	// delete fragments
	map<UInt32,Fragment*>::const_iterator it;
	for(it=_fragments.begin();it!=_fragments.end();++it)
		delete it->second;

	// delete receive buffer
	if(_pBuffer)
		delete _pBuffer;
}

void Flow::complete() {
	if(!_completed && id>0)
		DEBUG("Flow '%u' consumed",id);
	writer._bound = false;
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
	ack.write8(noMore() ? 0x00 : 0x3f);
	ack.write7BitValue(stage);

	writer.flush();
	if(_completed)
		writer.close();
}

void Flow::fragmentHandler(UInt32 stage,UInt32 deltaNAck,PacketReader& fragment,UInt8 flags) {
	if(_completed)
		return;

	UInt32 nextStage = this->stage+1;

	if(deltaNAck>0) {

		if(deltaNAck>stage) {
			ERROR("DeltaNAck %u superior than stage %u on the flow %u",deltaNAck,stage,id);
			deltaNAck=stage;
		}

		if( this->stage < (stage-deltaNAck) || flags&MESSAGE_ABANDONMENT) {
			
			map<UInt32,Fragment*>::iterator it=_fragments.begin();
			while(it!=_fragments.end()) {
				if( it->first > stage)
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
	}

	if(nextStage > stage) {
		DEBUG("Stage %u on flow %u has already been received",stage,id);
		return;
	}

	if(stage>nextStage) {
		// not following stage!
		if(_fragments.find(stage) == _fragments.end())
			_fragments[stage] = new Fragment(fragment,flags);
		else
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
		WARN("%u packets lost on flow %u",stage-this->stage-1,id);
		(UInt32&)this->stage = stage;
		if(_pBuffer) {
			delete _pBuffer;
			_pBuffer = NULL;
		}
		if(flags&MESSAGE_WITH_BEFOREPART)
			return;
	} else
		(UInt32&)this->stage = stage;
	
	PacketReader* pMessage(NULL);
	if(flags&MESSAGE_WITH_BEFOREPART){
		if(!_pBuffer) {
			WARN("A received message tells to have a 'beforepart' and nevertheless partbuffer is empty, certainly some packets were lost");
			delete _pBuffer;
			_pBuffer = NULL;
			return;
		}
		
		Buffer<UInt8>* pOldBuffer = _pBuffer;
		_pBuffer = new Buffer<UInt8>(pOldBuffer->size() + fragment.available());
		memcpy(_pBuffer->begin(),pOldBuffer->begin(),pOldBuffer->size());
		memcpy(_pBuffer->begin()+pOldBuffer->size(),fragment.current(),fragment.available());
		delete pOldBuffer;

		if(flags&MESSAGE_WITH_AFTERPART)
			return;

		pMessage = new PacketReader(_pBuffer->begin(),_pBuffer->size());
	} else if(flags&MESSAGE_WITH_AFTERPART) {
		if(_pBuffer) {
			ERROR("A received message tells to have not 'beforepart' and nevertheless partbuffer exists");
			delete _pBuffer;
		}
		_pBuffer = new Buffer<UInt8>(fragment.available());
		memcpy(_pBuffer->begin(),fragment.current(),_pBuffer->size());
		return;
	}
	if(!pMessage)
		pMessage = new PacketReader(fragment);

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

	if(_pBuffer) {
		delete _pBuffer;
		_pBuffer=NULL;
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
