/* 
	Copyright 2010 cumulus.dev@gmail.com
 
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

#include "FlowConnection.h"
#include "AMFWriter.h"
#include "Logs.h"

using namespace Poco;

namespace Cumulus {


FlowConnection::FlowConnection(Poco::UInt8 id) : Flow(id) {
}

FlowConnection::~FlowConnection() {
}

int FlowConnection::requestHandler(UInt8 stage,PacketReader& request,PacketWriter& response) {

	char buff[MAX_SIZE_MSG];
	AMFWriter amf(response);


	switch(stage){
		case 0x01:
			response.write8(0x80);
			response.write8(id);response.write8(stage);

			request.readRaw(buff,7);
			response.writeRaw(buff,7);
			response.writeRaw("\x02\x0a\x02");
			request.readRaw(buff,6);
			response.writeRaw(buff,6);
			amf.write("_result");
			amf.write(1);
			amf.writeNull();

			amf.beginObject();
			amf.writeObjectProperty("objectEncoding",3);
			amf.writeObjectProperty("description","Connection succeeded");
			amf.writeObjectProperty("level","status");
			amf.writeObjectProperty("code","NetConnection.Connect.Success");
			amf.endObject();

			return 0x10;
		case 0x02:
			response.write8(0);
			response.write8(id);response.write8(stage);
			response.writeRaw("\x01\x04\x00\x00\x00\x00\x00\x29\x00\x00\x3a\x98\x00\x00\x27\x10",16);
			return 0x10;
		default:
			ERROR("Unkown FlowConnection stage '%02x'",stage);
	}

	return 0;
}

} // namespace Cumulus