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

FlowConnection::FlowConnection(UInt32 id,Peer& peer,Handler& handler,BandWriter& band) : Flow(id,Signature,_Name,peer,handler,band) {
	(bool&)writer.critical = true;
}

FlowConnection::~FlowConnection() {
	// delete stream index remaining (which have not had time to send a 'destroyStream' message)
	set<UInt32>::const_iterator it;
	for(it=_streamIndex.begin();it!=_streamIndex.end();++it)
		handler.streams.destroy(*it);
}

void FlowConnection::messageHandler(const std::string& name,AMFReader& message) {
	
	if(name=="connect") {
		AMFObject obj;
		message.readObject(obj);
		((URI&)peer.swfUrl) = obj.getString("swfUrl","");
		((URI&)peer.pageUrl) = obj.getString("pageUrl","");

		// Don't support AMF0 forced on NetConnection object because impossible to exchange custome data (ByteArray written impossible)
		// But it's not a pb because NetConnection RTMFP works since flash player 10.0 only (which supports AMF3)
		if(obj.getDouble("objectEncoding")==0)
			throw Exception("ObjectEncoding client must be AMF3 and not AMF0");

		// Check if the client is authorized
		++((UInt32&)handler.count);
		((Peer::PeerState&)peer.state) = Peer::REJECTED;
		if(!handler.onConnection(peer,writer))
			throw Exception("Client rejected");
		
		((Peer::PeerState&)peer.state) = Peer::ACCEPTED;

		AMFObjectWriter response(writer.writeSuccessResponse("Connect.Success","Connection succeeded"));
		response.write("objectEncoding",3);
		response.write("data",peer.data);

	} else if(name == "setPeerInfo") {

		list<Address> address;
		string addr;
		while(message.available()) {
			message.read(addr); // private host
			address.push_back(addr);
		}
		peer.setPrivateAddress(address);
		
		BinaryWriter& response(writer.writeRawMessage());
		response.write16(0x29); // Unknown!
		response.write32(handler.keepAliveServer);
		response.write32(handler.keepAlivePeer);

	} else if(name == "initStream") {
		// TODO?
	} else if(name == "createStream") {

		AMFWriter& response(writer.writeAMFResult());
		response.writeNumber(*_streamIndex.insert(handler.streams.create()).first);

	} else if(name == "deleteStream") {
		UInt32 index = (UInt32)message.readNumber();
		_streamIndex.erase(index);
		handler.streams.destroy(index);
	} else {
		if(!handler.onMessage(peer,name,message,writer))
			writer.writeErrorResponse("Call.Failed","Method '" + name + "' not found");
	}
}



} // namespace Cumulus
