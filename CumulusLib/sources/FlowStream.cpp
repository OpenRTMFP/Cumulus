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

#include "FlowStream.h"
#include "Logs.h"
#include "Poco/StreamCopier.h"

using namespace std;
using namespace Poco;


namespace Cumulus {

string FlowStream::s_signature("\x00\x54\x43\x04\x01",5);
string FlowStream::s_name("NetStream");

FlowStream::FlowStream(UInt8 id,Peer& peer,Session& session,ServerHandler& serverHandler) : Flow(id,s_signature,s_name,peer,session,serverHandler) {
}

FlowStream::~FlowStream() {
}


void FlowStream::rawHandler(PacketReader& data) {
	UInt8 type = data.read8();
	DEBUG("%02x",type);
	if(type==0x09 || type==0x08) { // video or sound
		if(serverHandler.pFlowTest) {
			MessageWriter& packet = serverHandler.pFlowTest->writeRawMessage(true);
			packet.write8(type);
			StreamCopier::copyStream(data.stream(),packet.stream());
			serverHandler.pFlowTest->flush();
		}
	}
}

void FlowStream::complete() {
	Flow::complete();
	if(serverHandler.pFlowTest==this)
		((ServerHandler&)serverHandler).pFlowTest = NULL;
}

void FlowStream::messageHandler(const string& name,AMFReader& message) {

	if(name=="publish") {
		// TODO add a failed scenario? or "badname" scenario?
		string nameStream,typeStream;
		message.read(nameStream);
		if(message.available())
			message.read(typeStream);
		writeStatusResponse("Start","'" + nameStream +"' is now published");

	} else if(name=="play") {
		// TODO add a failed scenario? or "badname" scenario?
		string nameStream;
		message.read(nameStream);
		writeStatusResponse("Start","Started playing '" + nameStream +"'");
		((ServerHandler&)serverHandler).pFlowTest = this;
	} else
		Flow::messageHandler(name,message);

}

} // namespace Cumulus
