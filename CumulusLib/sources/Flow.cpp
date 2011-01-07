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
#include "string.h"
#include "Logs.h"

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
		++_time;
		if(_time>=_cycle) {
			_time=0;
			++_cycle;
			if(_cycle==7)
				throw exception("Send message failed");
			DEBUG("Repeat message, try %x",_cycle+1);
			response.write8(0x10);
			response.write16(_size);
			response.writeRaw(_buffer,_size);
			return true;
		}
		return false;
	}

	UInt8		stage;
	
private:
	Int8		_cycle;
	UInt8		_time;
	int			_size;
	UInt8*		_buffer;
};


Flow::Flow(Peer& peer,ServerData& data) : _stage(0),peer(peer),data(data),_pLastResponse(NULL),_consumed(0) {
}

Flow::~Flow() {
	// delete last response
	if(_pLastResponse)
		delete _pLastResponse;
}

bool Flow::consumed() {
	UInt8 stage = 0;
	UInt64 consumed = _consumed;
	while(stage<maxStage()) {
		if(!(consumed&0x01))
			return false;
		consumed>>=1;
		++stage;
	}
	return true;
}

void Flow::stageCompleted(UInt8 stage) {
	_stage = stage; // progress stage
	_consumed |= (((UInt64)1)<<((UInt64)stage-1));
}

bool Flow::lastResponse(PacketWriter& response) {
	if(!_pLastResponse)
		return false;
	bool result = false;
	try {
		result = _pLastResponse->response(response);
	} catch(const exception&) {
		delete _pLastResponse;
		_pLastResponse = NULL;
		throw;
	}
	return result;
}

bool Flow::request(UInt8 stage,PacketReader& request,PacketWriter& response) {
	// We can considerate that a request stage+1 pays as a ack for requete stage
	if(stage>0)
		stageCompleted(stage-1);

	int pos = response.position()-4; // 4 for firstFlag, idFlow, stage and second Flag write in Session.cpp
	bool answer = requestHandler(stage,request,response);
	if(!answer) {
		stageCompleted(stage); // stage complete because there is no response
		return false;
	}

	// Mem the last response (for the correspondant stage flow)
	if(_pLastResponse)
		delete _pLastResponse;
	_pLastResponse = new Response(stage,response.begin()+pos,response.length()-pos,_buffer);
	
	return true;
}

void Flow::acknowledgment(Poco::UInt8 stage) {
	if(!_pLastResponse)
		return;
	// Ack!
	if(_pLastResponse->stage == stage) {
		delete _pLastResponse;
		_pLastResponse = NULL;
		stageCompleted(stage);
	}
}	

} // namespace Cumulus