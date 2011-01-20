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

#include "FlowConnection.h"
#include "AMFReader.h"
#include "AMFWriter.h"
#include "Logs.h"


using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {


FlowConnection::FlowConnection(Peer& peer,ServerData& data) : Flow(peer,data) {
}

FlowConnection::~FlowConnection() {
}

Flow::StageFlow FlowConnection::requestHandler(UInt8 stage,PacketReader& request,PacketWriter& response) {

	char buff1[MAX_SIZE_MSG];
	char buff2[MAX_SIZE_MSG];
	AMFReader reader(request);
	AMFWriter writer(response);

	switch(stage){
		case 0x01: {
			request.readRaw(buff1,6);
			request.readRaw(buff2,6);

			string tmp;
			reader.read(tmp);
			reader.readNumber();
			// fill peer infos!
			AMFObject obj;
			reader.readObject(obj);
			((URI&)peer.swfUrl) = obj.getString("swfUrl","");
			((URI&)peer.pageUrl) = obj.getString("pageUrl","");

			// Check if the client is authorized
			if(!data.auth(peer))
				break;

			response.writeRaw(buff1,6);
			response.writeRaw("\x02\x0a\x02",3);
			response.writeRaw(buff2,6);
			writer.write("_result");
			writer.writeNumber(1);
			writer.writeNull();

			writer.beginObject();
			writer.writeObjectProperty("objectEncoding",3);
			writer.writeObjectProperty("data",peer.data);
			writer.writeObjectProperty("level","status");
			writer.writeObjectProperty("code","NetConnection.Connect.Success");
			writer.endObject();
			break;
		}
		case 0x02: {

			request.next(6); // unknown, 11 00 00 03 96 00

			string tmp; 
			reader.read(tmp); // "setPeerInfo"

			reader.readNumber(); // Unknown, always equals at 0
			reader.readNull();

			while(reader.available()) {
				reader.read(tmp); // private host
				try {
					SocketAddress addr(tmp);
					peer.addPrivateAddress(addr);
				} catch(Exception& ex) {
					ERROR("Incorrect peer address : %s",ex.displayText().c_str());
				}
			}

			response.writeRaw("\x04\x00\x00\x00\x00\x00\x29\x00\x00",9); // Unknown!
			response.write16(data.keepAliveServer);
			response.write16(0); // Unknown!
			response.write16(data.keepAlivePeer);
			return MAX;
		}
		default:
			ERROR("Unkown FlowNetConnection stage '%02x'",stage);
	}

	return STOP;
}

} // namespace Cumulus