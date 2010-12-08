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

#include "FlowNetStream.h"
#include "AMFReader.h"
#include "AMFWriter.h"
#include "Logs.h"
#include "Poco/HexBinaryDecoder.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {


FlowNetStream::FlowNetStream(Poco::UInt8 id,const BLOB& peerId,const SocketAddress& peerAddress,Database& database) : Flow(id,peerId,peerAddress,database) {
}

FlowNetStream::~FlowNetStream() {
}

int FlowNetStream::requestHandler(UInt8 stage,PacketReader& request,PacketWriter& response) {
	char buff[MAX_SIZE_MSG];
	AMFReader reader(request);
	AMFWriter writer(response);

	switch(stage){
		case 0x01: {
			request.readRaw(buff,6);

			string type;
			reader.read(type);
			
			request.skip(3); // 0x10 G :

			request.skip(16); // 01010102010c290e

			UInt8 groupId[80];
			request.readRaw(groupId,sizeof(groupId));
			MemoryInputStream mis((char*)groupId,sizeof(groupId));
			HexBinaryDecoder(mis).read((char*)groupId,40);
			
			BLOB peerOwner;
			if(database.addGroup(peerId,BLOB(groupId,40),peerOwner))
				return 0;

			response.writeRaw(buff,6);
			response.writeRaw("\x03\x00\x0b",3);
			response.writeRaw(peerOwner.begin(),peerOwner.size());

			return 0x10;
		}
		default:
			ERROR("Unkown FlowNetStream stage '%02x'",stage);
	}

	return 0;
}

} // namespace Cumulus