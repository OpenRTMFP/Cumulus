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
	Response(Poco::UInt8 stage,const UInt8* data,UInt16 size,UInt8* buffer) : _size(size),stage(stage),_buffer(buffer),_time(0),_cycle(-1) {
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

	const UInt8		stage;
	
private:
	Timestamp	_timeCreation;
	Int8		_cycle;
	UInt8		_time;
	int			_size;
	UInt8*		_buffer;
};


Flow::Flow(Peer& peer,ServerData& data) : _stage(0),peer(peer),data(data),_pLastResponse(NULL),_consumed(0),_maxStage(0) {
}

Flow::~Flow() {
	// delete last response
	if(_pLastResponse)
		delete _pLastResponse;
}

bool Flow::consumed() {
	if(_maxStage==0)
		return false;
	UInt8 stage = 0;
	UInt64 consumed = _consumed;
	while(stage<_maxStage) {
		if(!(consumed&0x01))
			return false;
		consumed>>=1;
		++stage;
	}
	return true;
}

void Flow::stageCompleted(UInt8 stage) {
	_consumed |= (((UInt64)1)<<((UInt64)stage-1));
	if(stage>_stage) // progress stage
		_stage = stage;
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
	// We can considerate that a request stage+1 pays as a ack for requete stage
	if(stage>0)
		stageCompleted(stage-1);

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
		++stage;
		first = false;
	}
	--stage;

	if(_pLastResponse)
		delete _pLastResponse;

	bool answer = pos<response.position();
	if(answer)
		_pLastResponse = new Response(stage,response.begin()+pos,response.length()-pos,_buffer); // Mem the last response (for the correspondant stage flow)
	else
		stageCompleted(stage); // stage complete because there is no responsereturn false;

	return answer;
}

void Flow::acknowledgment(Poco::UInt8 stage) {
	stageCompleted(stage);
	if(!_pLastResponse)
		return;
	// Ack!
	if(this->stage() >= _pLastResponse->stage) {
		delete _pLastResponse;
		_pLastResponse = NULL;
	}
}	

} // namespace Cumulus