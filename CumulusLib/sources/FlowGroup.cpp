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


FlowGroup::FlowGroup(Peer& peer,ServerData& data) : Flow(peer,data),_pGroup(NULL) {
}

FlowGroup::~FlowGroup() {
}


Flow::StageFlow FlowGroup::requestHandler(UInt8 stage,PacketReader& request,PacketWriter& response) {
	char buff[MAX_SIZE_MSG];
	AMFReader reader(request);
	AMFWriter writer(response);

	if(stage == 0x01) {
		request.readRaw(buff,6);
		request.next(3);

		UInt32 size = request.read7BitValue();

		vector<UInt8> groupId(size);
		request.readRaw(&groupId[0],size);

		_pGroup = &data.group(groupId);

		_pGroup->bestPeers(_bestPeers,peer);

		_pGroup->addPeer(peer);
		if(_bestPeers.empty())
			return STOP;

		response.writeRaw(buff,6);
		response.write8(0x03);
		response.write16(0x0b);
		response.writeRaw(_bestPeers.front()->id,32);
		_bestPeers.pop_front();
		return _bestPeers.empty() ? STOP : NEXT;

	} else if(!_bestPeers.empty()) {
		response.write16(0x0b);
		response.writeRaw(_bestPeers.front()->id,32);
		_bestPeers.pop_front();
		return _bestPeers.empty() ? STOP : NEXT;

	} else {
		// delete member of group
		DEBUG("Group closed")
		if(_pGroup)
			_pGroup->removePeer(peer);
		return MAX;
	}
}

} // namespace Cumulus