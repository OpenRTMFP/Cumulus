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
#include "Util.h"


using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

string FlowConnection::Signature("\x00\x54\x43\x04\x00",5);
string FlowConnection::_Name("NetConnection");

FlowConnection::FlowConnection(UInt32 id,Peer& peer,Invoker& invoker,BandWriter& band) : Flow(id,Signature,_Name,peer,invoker,band) {
	(bool&)writer.critical = true;
}

FlowConnection::~FlowConnection() {
	// delete stream index remaining (which have not had time to send a 'destroyStream' message)
	set<UInt32>::const_iterator it;
	for(it=_streamIndex.begin();it!=_streamIndex.end();++it)
		invoker._streams.destroy(*it);
}

void FlowConnection::messageHandler(const std::string& name,AMFReader& message) {
	if(name=="connect") {
		message.stopReferencing();
		AMFSimpleObject obj;
		message.readSimpleObject(obj);
		message.startReferencing();
		((URI&)peer.swfUrl) = obj.getString("swfUrl","");
		((URI&)peer.pageUrl) = obj.getString("pageUrl","");
		((string&)peer.flashVersion) = obj.getString("flashVer","");

		// Don't support AMF0 forced on NetConnection object because AMFWriter writes in AMF3 format
		// But it's not a pb because NetConnection RTMFP works since flash player 10.0 only (which supports AMF3)
		if(obj.getNumber("objectEncoding",0)==0) {
			writer.writeErrorResponse("Connect.Error","ObjectEncoding client must be in a AMF3 format (not AMF0)");
			return;
		}

		// Check if the client is authorized
		peer.setFlowWriter(&writer);
		UInt32 queue = writer.queue();
		bool accept=true;
		{
			AMFObjectWriter response(writer.writeSuccessResponse("Connect.Success","Connection succeeded"));
			response.write("objectEncoding",3.0);
			accept = peer.onConnection(message,response);
		}
		if(!accept) {
			writer.cancel(queue);
			peer.close();
		}

	} else if(name == "setPeerInfo") {

		peer.addresses.erase(++peer.addresses.begin(),peer.addresses.end());
		string addr;
		while(message.available()) {
			message.read(addr); // private host
			peer.addresses.push_back(addr);
		}
		
		BinaryWriter& response(writer.writeRawMessage());
		response.write16(0x29); // Unknown!
		response.write32(invoker.keepAliveServer);
		response.write32(invoker.keepAlivePeer);

	} else if(name == "initStream") {
		// TODO?
	} else if(name == "createStream") {

		AMFWriter& response(writer.writeAMFResult());
		response.writeInteger(*_streamIndex.insert(invoker._streams.create()).first);

	} else if(name == "deleteStream") {
		UInt32 index = (UInt32)message.readNumber();
		_streamIndex.erase(index);
		invoker._streams.destroy(index);
	} else {
		if(!peer.onMessage(name,message))
			writer.writeErrorResponse("Call.Failed","Method '" + name + "' not found");
	}
}



} // namespace Cumulus
