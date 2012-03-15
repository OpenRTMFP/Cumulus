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


Flow::Flow(UInt64 id,const string& signature,const string& name,Peer& peer,Invoker& invoker,BandWriter& band) : id(id),stage(0),peer(peer),invoker(invoker),_completed(false),_pPacket(NULL),_band(band),writer(*new FlowWriter(signature,band)) {
	if(writer.flowId==0)
		((UInt64&)writer.flowId)=id;
	// create code prefix for a possible response
	writer._obj.assign(name);
}

Flow::~Flow() {
	complete();
	writer.close();
}

void Flow::complete() {
	if(_completed)
		return;

	if(!writer.signature.empty()) // writer.signature.empty() == FlowNull instance, not display the message in FullNull case
		DEBUG("Flow %llu consumed",id);

	// delete fragments
	map<UInt64,Fragment*>::const_iterator it;
	for(it=_fragments.begin();it!=_fragments.end();++it)
		delete it->second;
	_fragments.clear();

	// delete receive buffer
	if(_pPacket) {
		delete _pPacket;
		_pPacket=NULL;
	}

	_completed=true;
}

void Flow::fail(const string& error) {
	ERROR("Flow %llu failed : %s",id,error.c_str());
	if(!_completed) {
		BinaryWriter& writer = _band.writeMessage(0x5e,Util::Get7BitValueSize(id)+1);
		writer.write7BitLongValue(id);
		writer.write8(0); // unknown
	}
}

Message::Type Flow::unpack(PacketReader& reader) {
	if(reader.available()==0)
		return Message::EMPTY;
	Message::Type type = (Message::Type)reader.read8();
	switch(type) {
		// amf content
		case 0x11:
			reader.next(1);
		case Message::AMF_WITH_HANDLER:
			reader.next(4);
			return Message::AMF_WITH_HANDLER;
		case Message::AMF:
			reader.next(5);
		case Message::AUDIO:
		case Message::VIDEO:
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

	// Lost informations!
	UInt32 size = 0;
	list<UInt64> lost;
	UInt64 current=stage;
	UInt32 count=0;
	map<UInt64,Fragment*>::const_iterator it=_fragments.begin();
	while(it!=_fragments.end()) {
		current = it->first-current-2;
		size += Util::Get7BitValueSize(current);
		lost.push_back(current);
		current = it->first;
		while(++it!=_fragments.end() && it->first==(++current))
			++count;
		size += Util::Get7BitValueSize(count);
		lost.push_back(count);
		--current;
		count=0;
	}

	UInt32 bufferSize = _pPacket ? ((_pPacket->fragments>0x3F00) ? 0 : (0x3F00-_pPacket->fragments)) : 0x7F;
	if(writer.signature.empty())
		bufferSize=0;

	PacketWriter& ack = _band.writeMessage(0x51,Util::Get7BitValueSize(id)+Util::Get7BitValueSize(bufferSize)+Util::Get7BitValueSize(stage)+size);
	UInt32 pos = ack.position();
	ack.write7BitLongValue(id);
	ack.write7BitValue(bufferSize);
	ack.write7BitLongValue(stage);

	list<UInt64>::const_iterator it2;
	for(it2=lost.begin();it2!=lost.end();++it2)
		ack.write7BitLongValue(*it2);

	commitHandler();
	writer.flush();
}

void Flow::fragmentHandler(UInt64 stage,UInt64 deltaNAck,PacketReader& fragment,UInt8 flags) {
	if(_completed)
		return;

//	TRACE("Flow %llu stage %llu",id,stage);

	UInt64 nextStage = this->stage+1;

	if(stage < nextStage) {
		DEBUG("Stage %llu on flow %llu has already been received",stage,id);
		return;
	}

	if(deltaNAck>stage) {
		WARN("DeltaNAck %llu superior to stage %llu on flow %llu",deltaNAck,stage,id);
		deltaNAck=stage;
	}
	
	if(this->stage < (stage-deltaNAck)) {
		map<UInt64,Fragment*>::iterator it=_fragments.begin();
		while(it!=_fragments.end()) {
			if( it->first > stage) 
				break;
			// leave all stages <= stage
			PacketReader reader(it->second->begin(),it->second->size());
			fragmentSortedHandler(it->first,reader,it->second->flags);
			if(it->second->flags&MESSAGE_END) {
				complete();
				return; // to prevent a crash bug!! (double fragments deletion)
			}
			delete it->second;
			_fragments.erase(it++);
		}

		nextStage = stage;
	}
	
	if(stage>nextStage) {
		// not following stage, bufferizes the stage
		map<UInt64,Fragment*>::iterator it = _fragments.lower_bound(stage);
		if(it==_fragments.end() || it->first!=stage) {
			if(it!=_fragments.begin())
				--it;
			_fragments.insert(it,pair<UInt64,Fragment*>(stage,new Fragment(fragment,flags)));
			if(_fragments.size()>100)
				DEBUG("_fragments.size()=%lu",_fragments.size()); 
		} else
			DEBUG("Stage %llu on flow %llu has already been received",stage,id);
	} else {
		fragmentSortedHandler(nextStage++,fragment,flags);
		if(flags&MESSAGE_END)
			complete();
		map<UInt64,Fragment*>::iterator it=_fragments.begin();
		while(it!=_fragments.end()) {
			if( it->first > nextStage)
				break;
			PacketReader reader(it->second->begin(),it->second->size());
			fragmentSortedHandler(nextStage++,reader,it->second->flags);
			if(it->second->flags&MESSAGE_END) {
				complete();
				return; // to prevent a crash bug!! (double fragments deletion)
			}
			delete it->second;
			_fragments.erase(it++);
		}

	}
}

void Flow::fragmentSortedHandler(UInt64 stage,PacketReader& fragment,UInt8 flags) {
	if(stage<=this->stage) {
		ERROR("Stage %llu not sorted on flow %llu",stage,id);
		return;
	}
	if(stage>(this->stage+1)) {
		// not following stage!
		UInt32 lostCount = (UInt32)(stage-this->stage-1);
		(UInt64&)this->stage = stage;
		if(_pPacket) {
			delete _pPacket;
			_pPacket = NULL;
		}
		if(flags&MESSAGE_WITH_BEFOREPART) {
			lostFragmentsHandler(lostCount+1);
			return;
		}
		lostFragmentsHandler(lostCount);
	} else
		(UInt64&)this->stage = stage;

	// If MESSAGE_ABANDONMENT, content is not the right normal content!
	if(flags&MESSAGE_ABANDONMENT) {
		if(_pPacket) {
			delete _pPacket;
			_pPacket = NULL;
		}
		return;
	}

	PacketReader* pMessage(&fragment);
	if(flags&MESSAGE_WITH_BEFOREPART){
		if(!_pPacket) {
			WARN("A received message tells to have a 'beforepart' and nevertheless partbuffer is empty, certainly some packets were lost");
			lostFragmentsHandler(1);
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
			lostFragmentsHandler(_pPacket->fragments);
			delete _pPacket;
		}
		_pPacket = new Packet(fragment);
		return;
	}

	Message::Type type = unpack(*pMessage);

	if(type!=Message::EMPTY) {
		writer._callbackHandle = 0;
		string name;
		AMFReader amf(*pMessage);
		if(type==Message::AMF_WITH_HANDLER || type==Message::AMF) {
			amf.read(name);
			if(type==Message::AMF_WITH_HANDLER) {
				writer._callbackHandle = amf.readNumber();
				if(amf.followingType()==AMF::Null)
					amf.readNull();
			}
		}

		try {
			switch(type) {
				case Message::AMF_WITH_HANDLER:
				case Message::AMF:
					messageHandler(name,amf);
					break;
				case Message::AUDIO:
					audioHandler(*pMessage);
					break;
				case Message::VIDEO:
					videoHandler(*pMessage);
					break;
				default:
					rawHandler(type,*pMessage);
			}
		} catch(Exception& ex) {
			_error = "flow error, " + ex.displayText();
		} catch(exception& ex) {
			_error = string("flow error, ") + ex.what();
		} catch(...) {
			_error = "Unknown flow error";
		}
	}
	writer._callbackHandle = 0;

	if(_pPacket) {
		delete _pPacket;
		_pPacket=NULL;
	}
	
}

void Flow::messageHandler(const std::string& name,AMFReader& message) {
	ERROR("Message '%s' unknown for flow %llu",name.c_str(),id);
}
void Flow::rawHandler(UInt8 type,PacketReader& data) {
	ERROR("Raw message %s unknown for flow %llu",Util::FormatHex(data.current(),data.available()).c_str(),id);
}
void Flow::audioHandler(PacketReader& packet) {
	ERROR("Audio packet untreated for flow %llu",id);
}
void Flow::videoHandler(PacketReader& packet) {
	ERROR("Video packet untreated for flow %llu",id);
}
void Flow::lostFragmentsHandler(UInt32 count) {
	INFO("%u fragments lost on flow %llu",count,id);
}



} // namespace Cumulus
