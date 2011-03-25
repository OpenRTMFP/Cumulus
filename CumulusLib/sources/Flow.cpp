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


Flow::Flow(const string& name,Peer& peer,ServerHandler& serverHandler) : _stage(0),peer(peer),serverHandler(serverHandler),_pLastResponse(NULL),_maxStage(0),_name(name) {
	
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

void Flow::acknowledgment(Poco::UInt8 stage) {
	if(stage > _stage) {
		WARN("A acknowledgment superior than the current stage has been sent : '%02x' instead of '%02x' ",stage,_stage);
		return;
	}
	if(!_pLastResponse)
		return;
	// Ack!
	delete _pLastResponse;
	_pLastResponse = NULL;
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
			CRITIC("Unpacking flag '%02x' unknown",flag);
		case 0x04:
			reader.next(4);
		case 0x01:
			;
	}
	return true;
}

bool Flow::request(UInt8 stage,PacketReader& request,PacketWriter& response) {
	if(stage==_stage) {
		INFO("Flow stage '%02x' has already been received",stage);
		return false;
	} else if(stage<_stage) {
		WARN("A flow stage '%02x' inferior to current stage '%02x' received",stage,_stage);
		return false;
	}

	_stage = stage;

	bool isRaw = unpack(request);
	double callbackHandle = 0;
	string name;
	AMFReader reader(request);
	if(!isRaw) {
		reader.read(name);
		callbackHandle = reader.readNumber();
		reader.skipNull();
	}
	
	bool answer = false;
	{
		ResponseWriter responseWriter(response,callbackHandle,_name,name);
		bool maxStage = isRaw ? rawHandler(_stage,request,responseWriter) : requestHandler(name,reader,responseWriter);
		responseWriter.flush();
		answer = responseWriter.count()>0;
		if(answer)
			_stage += responseWriter.count()-1;
		if(maxStage)
			_maxStage = _stage;
	}

	if(_pLastResponse) {
		delete _pLastResponse;
		_pLastResponse = NULL;
	}

	if(answer)
		_pLastResponse = new Response(response.begin()+response.position(),response.length()-response.position(),_buffer); // Mem the last response

	// Consume
	response.reset(response.length());

	return answer;
}

bool Flow::requestHandler(const std::string& name,AMFReader& request,ResponseWriter& responseWriter) {
	ERROR("Request '%s' unknown",name.c_str());
	return false;
}
bool Flow::rawHandler(Poco::UInt8 stage,PacketReader& request,ResponseWriter& responseWriter) {
	ERROR("Request of stage '%02x' unknown",stage);
	return false;
}



} // namespace Cumulus
