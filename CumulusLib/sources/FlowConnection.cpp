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

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

string FlowConnection::s_signature("\x00\x54\x43\x04\x00",5);
string FlowConnection::s_name("NetConnection");

FlowConnection::FlowConnection(Peer& peer,ServerHandler& serverHandler) : Flow(s_name,peer,serverHandler) {
}

FlowConnection::~FlowConnection() {
}

bool FlowConnection::requestHandler(const std::string& name,AMFReader& request,ResponseWriter& responseWriter) {
	
	if(name=="connect") {

		AMFObject obj;
		request.readObject(obj);
		((URI&)peer.swfUrl) = obj.getString("swfUrl","");
		((URI&)peer.pageUrl) = obj.getString("pageUrl","");

		((Client::ClientState&)peer.state) = Client::REJECTED;

		// Don't support AMF0 forced on NetConnection object because impossible to exchange custome data (ByteArray written impossible)
		// But it's not a pb because NetConnection RTMFP works since flash player 10.0 only (which supports AMF3)
		if(obj.getDouble("objectEncoding")==0)
			return false;

		// Check if the client is authorized
		if(!serverHandler.connection(peer))
			return false;
		
		((Client::ClientState&)peer.state) = Client::ACCEPTED;

		AMFObjectWriter response(responseWriter.writeSuccessResponse("Connection succeeded"));
		response.write("objectEncoding",3);
		response.write("data",peer.data);

	} else if(name == "setPeerInfo") {

		list<Address> address;
		string addr;
		while(request.available()) {
			request.read(addr); // private host
			address.push_back(addr);
		}
		peer.setPrivateAddress(address);
		
		PacketWriter& response(responseWriter.writeRawResponse());
		response.write16(0x29); // Unknown!
		response.write32(serverHandler.keepAliveServer);
		response.write32(serverHandler.keepAlivePeer);

	} else if(name == "createStream") {

		AMFWriter& response(responseWriter.writeAMFResponse());
		response.writeNumber(1); // TODO, id stream!

	} else
		responseWriter.writeErrorResponse("Method not found (" + name + ")");

	return false;
}



} // namespace Cumulus
