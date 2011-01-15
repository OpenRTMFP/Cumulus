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

#include "FlowGroup.h"
#include "AMFReader.h"
#include "AMFWriter.h"
#include "Logs.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {


FlowGroup::FlowGroup(Peer& peer,ServerData& data) : Flow(peer,data),_pGroup(NULL),_memberRemoved(false) {
}

FlowGroup::~FlowGroup() {
}

UInt8 FlowGroup::maxStage() {
	return 0x02;
}

bool FlowGroup::requestHandler(UInt8 stage,PacketReader& request,PacketWriter& response) {
	char buff[MAX_SIZE_MSG];
	AMFReader reader(request);
	AMFWriter writer(response);

	switch(stage){
		case 0x01: {
			if(_memberRemoved) {
				WARN("Group member removed, no group subscription possible after that");
				return false;
			}
			request.readRaw(buff,6);
			request.next(3);

			UInt32 size = request.read7BitValue();

			vector<UInt8> groupId(size);
			request.readRaw(&groupId[0],size);

			_pGroup = &data.group(groupId);

			Peer* pPeer = _pGroup->bestPeer();
			_pGroup->addPeer(peer);
			if(!pPeer || (peer == *pPeer))
				return false;

			response.writeRaw(buff,6);
			response.writeRaw("\x03\x00\x0b",3);
			response.writeRaw(pPeer->id,32);

			return true;
		}
		case 0x02: {
			// delete member of group
			if(_memberRemoved) {
				WARN("Group member already removed");
			} else {
				if(_pGroup)
					_pGroup->removePeer(peer);
				_memberRemoved=true;
			}
			return false;
		}
		default:
			ERROR("Unkown FlowGroup stage '%02x'",stage);
	}

	return false;
}

} // namespace Cumulus