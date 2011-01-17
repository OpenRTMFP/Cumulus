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

#pragma once

#include "Cumulus.h"
#include "PacketReader.h"
#include "ServerData.h"
#include "Poco/Net/SocketAddress.h"

namespace Cumulus {

class Response;
class Flow
{
public:
	Flow(Peer& peer,ServerData& data);
	virtual ~Flow();

	bool request(Poco::UInt8 stage,PacketReader& request,PacketWriter& response);

	void acknowledgment(Poco::UInt8 stage);
	bool lastResponse(PacketWriter& response);
	bool consumed();
	virtual bool isNull();

	Peer&				peer;
	Poco::UInt8			stage();
	ServerData&			data;
	
	
private:
	virtual bool requestHandler(Poco::UInt8 stage,PacketReader& request,PacketWriter& response)=0;
	virtual Poco::UInt8 maxStage()=0;

	void stageCompleted(Poco::UInt8 stage);

	Poco::UInt8			_stage;
	Poco::UInt64		_consumed;
	Response*			_pLastResponse;
	Poco::UInt8			_buffer[MAX_SIZE_MSG];
};

inline Poco::UInt8 Flow::stage() {
	return _stage;
}

inline bool Flow::isNull() {
	return false;
}

} // namespace Cumulus
