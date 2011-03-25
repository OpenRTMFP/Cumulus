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
#include "Logs.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

string FlowGroup::s_signature("\x00\x47\x43",3);
string FlowGroup::s_name("NetGroup");

FlowGroup::FlowGroup(Peer& peer,ServerHandler& serverHandler) : Flow(s_name,peer,serverHandler),_pGroup(NULL) {
}

FlowGroup::~FlowGroup() {
}


bool FlowGroup::rawHandler(Poco::UInt8 stage,PacketReader& request,ResponseWriter& responseWriter) {

	if(stage == 0x01) {
		UInt32 size = request.read7BitValue();

		vector<UInt8> groupId(size);
		request.readRaw(&groupId[0],size);

		_pGroup = &serverHandler.group(groupId);

		_pGroup->bestPeers(_bestPeers,peer);

		_pGroup->addPeer(peer);
		if(_bestPeers.empty())
			return false;

		while(!_bestPeers.empty()) {
			PacketWriter& response(responseWriter.writeRawResponse(true));
		
			response.write8(0x0b); // unknown
			response.writeRaw(_bestPeers.front()->id,32);
			_bestPeers.pop_front();
		}

	} else {
		// delete member of group
		DEBUG("Group closed")
		if(_pGroup)
			_pGroup->removePeer(peer);
		return true;
	}
	return false;
}

} // namespace Cumulus
