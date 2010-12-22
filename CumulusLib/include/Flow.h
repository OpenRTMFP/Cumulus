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
#include <map>

namespace Cumulus {

class Response;
class Flow
{
public:
	Flow(Poco::UInt8 id,Peer& peer,ServerData& data);
	virtual ~Flow();

	int request(Poco::UInt8 stage,PacketReader& request,PacketWriter& response);

	void acknowledgment(Poco::UInt8 stage,bool ack);
	bool responseNotAck(PacketWriter& response);

	const Poco::UInt8	id;
	Peer&				peer;
	Poco::UInt8			stage() const;
	ServerData&			data;
	
	
private:
	virtual int requestHandler(Poco::UInt8 stage,PacketReader& request,PacketWriter& response)=0;

	Poco::UInt8						_stage;
	std::map<Poco::UInt8,Response*> _responses;
};

inline Poco::UInt8 Flow::stage() const {
	return _stage;
}

} // namespace Cumulus
