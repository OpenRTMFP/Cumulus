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

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

class Response {
public:
	Response(UInt8 id,PacketWriter& response) : _size(response.size()),acknowledgment(id!=0x10) { // Only request (0x10) must be acquitted
		memcpy(_response,response.begin(),_size);
	}
	
	~Response() {
	}

	void response(PacketWriter& response) {
		response.writeRaw(_response,_size);
	}

	bool		acknowledgment;
	
private:
	UInt8		_response[MAX_SIZE_MSG];
	int			_size;
};


Flow::Flow(UInt8 id,const BLOB& peerId,const SocketAddress& peerAddress,Database& database) : id(id),_stage(0),peerId(peerId),database(database),peerAddress(peerAddress) {
}

Flow::~Flow() {
	// delete responses
	map<Poco::UInt8,Response*>::const_iterator it;
	for(it=_responses.begin();it!=_responses.end();++it)
		delete it->second;
	_responses.clear();
}

bool Flow::responseNotAck(PacketWriter& response) {
	map<Poco::UInt8,Response*>::const_iterator it;
	Response* pResponse=NULL;
	UInt8 minStage=0xff;
	for(it=_responses.begin();it!=_responses.end();++it) {
		if(!it->second->acknowledgment && it->first<minStage)
			pResponse = it->second;
	}
	if(!pResponse)
		return false;
	pResponse->response(response);
	return true;
}

int Flow::request(UInt8 stage,PacketReader& request,PacketWriter& response) {
	int idResponse = requestHandler(stage,request,response);
	if(idResponse>=0)
		_stage = stage; // progress stage on response or not volontary response

	// Mem the last response (for the correspondant stage flow)
	if(_responses.find(stage)!=_responses.end())
		delete _responses[stage];
	_responses[stage] = new Response(idResponse,response);
	
	return idResponse;
}

void Flow::acknowledgment(Poco::UInt8 stage,bool ack) {
	if(_responses.find(stage)!=_responses.end())
		_responses[stage]->acknowledgment = ack;
}	

} // namespace Cumulus