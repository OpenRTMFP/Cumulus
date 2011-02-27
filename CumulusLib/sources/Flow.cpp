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
#include "Poco/Timestamp.h"
#include "string.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

class Response {
public:
	Response(const UInt8* data,UInt16 size,UInt8* buffer) : _size(size),_buffer(buffer),_time(0),_cycle(-1) {
		memcpy(buffer,data,size);
	}
	
	~Response() {
	}

	bool response(PacketWriter& response) {
		// Wait at least 1.5 sec before to begin the repeat cycle
		if(_time==0 && !_timeCreation.isElapsed(1500000))
			return false;
		++_time;
		if(_time>=_cycle) {
			_time=0;
			++_cycle;
			if(_cycle==7)
				throw Exception("Send message failed");
			DEBUG("Repeat message, try %02x",_cycle+1);
			response.writeRaw(_buffer,_size);
			return true;
		}
		return false;
	}
	
private:
	Timestamp	_timeCreation;
	Int8		_cycle;
	UInt8		_time;
	int			_size;
	UInt8*		_buffer;
};


Flow::Flow(Peer& peer,ServerData& data) : _stage(0),peer(peer),data(data),_pLastResponse(NULL),_maxStage(0) {
}

Flow::~Flow() {
	// delete last response
	if(_pLastResponse)
		delete _pLastResponse;
}

bool Flow::lastResponse(PacketWriter& response) {
	if(!_pLastResponse)
		return false;
	bool result = false;
	try {
		result = _pLastResponse->response(response);
	} catch(const Exception&) {
		delete _pLastResponse;
		_pLastResponse = NULL;
		throw;
	}
	return result;
}

void Flow::writeResponse(PacketWriter& packet,bool nestedResponse) {
	packet.write8(nestedResponse ? 0x11 : 0x10);
	int len = packet.length()-packet.position()-2;
	if(len<0)
		len = 0;
	packet.write16(len);
	packet.next(len);
}

bool Flow::request(UInt8 stage,PacketReader& request,PacketWriter& response) {
	if(stage==_stage) {
		INFO("Flow stage '%02x' has been resent",stage);
		return false;
	} else if(stage<_stage) {
		WARN("A flow stage '%02x' inferior to current stage '%02x' has been sent",stage,_stage);
		return false;
	}
	_stage = stage;

	response.reset(response.position()-7); // 7  for type, size, firstFlag, idFlow, stage and second Flag written in Session.cpp
	int pos = response.position();

	bool first = true;
	StageFlow stageFlow(NEXT);
	while(stageFlow == NEXT) {
		PacketWriter out(response,first ? 7 : 3);
		stageFlow = requestHandler(stage,request,out);
		if(stageFlow == NEXT || out.length()>response.length()) {
			out.flush();
			writeResponse(response,!first);
		} else
			out.clear();
		if(stageFlow == MAX)
			_maxStage = stage;
		++_stage;
		first = false;
	}
	--_stage;

	if(_pLastResponse) {
		delete _pLastResponse;
		_pLastResponse = NULL;
	}

	bool answer = pos<response.position();
	if(answer)
		_pLastResponse = new Response(response.begin()+pos,response.length()-pos,_buffer); // Mem the last response

	return answer;
}

void Flow::acknowledgment(Poco::UInt8 stage) {
	if(stage != _stage) {
		WARN("A acknowledgment for flow stage '%02x' has been sent whereas the current stage for this flow is '%02x' ",stage,_stage);
		return;
	}
	if(!_pLastResponse)
		return;
	// Ack!
	delete _pLastResponse;
	_pLastResponse = NULL;
}	

} // namespace Cumulus
