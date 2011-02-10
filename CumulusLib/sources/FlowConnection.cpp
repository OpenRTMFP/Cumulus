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
#include "Logs.h"
#include "AMFResponse.h"


using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {


FlowConnection::FlowConnection(Peer& peer,ServerData& data,ClientHandler* pClientHandler) : Flow(peer,data),_pClientHandler(pClientHandler) {
}

FlowConnection::~FlowConnection() {
}

Flow::StageFlow FlowConnection::requestHandler(UInt8 stage,PacketReader& request,PacketWriter& response) {

	char buff1[MAX_SIZE_MSG];
	char buff2[MAX_SIZE_MSG];
	AMFReader reader(request);
	AMFWriter writer(response);

	if(stage==0x01) {
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


		// Don't support AMF0 forced on NetConnection object because impossible to exchange custome data (ByteArray written impossible)
		// But it's not a pb because NetConnection RTMFP works since flash player 10.0 only (which supports AMF3)
		if(obj.getDouble("objectEncoding")==0)
			return STOP;

		// Check if the client is authorized
		if(!data.auth(peer))
			return STOP;

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

	} else {
		request.next(6); // unknown, 11 00 00 03 96 00

		string name; 
		reader.read(name);
		double handle = reader.readNumber(); // Unknown, always equals at 0
		reader.readNull();

		if(stage==0x02) {
			response.writeRaw("\x04\x00\x00\x00\x00\x00\x29\x00\x00",9); // Unknown!
			response.write16(data.keepAliveServer);
			response.write16(0); // Unknown!
			response.write16(data.keepAlivePeer);
		} else {
			response.writeRaw("\x11\x00\x00\x00\x00\x00",6); // Unknown!
		}

		callbackHandler(name,reader,handle,writer);
		
	}

	return STOP;
}


void FlowConnection::callbackHandler(const string& name,AMFReader& reader,double responderHandle,AMFWriter& writer) {

	if(name == "setPeerInfo") {
		list<Address> address;
		string addr;
		while(reader.available()) {
			reader.read(addr); // private host
			address.push_back(addr);
		}
		peer.setPrivateAddress(address);
	} else 
		AMFResponse response(writer,responderHandle,"Method not found (" + name + ")");
	
}

} // namespace Cumulus