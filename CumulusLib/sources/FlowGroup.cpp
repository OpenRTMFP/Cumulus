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

FlowGroup::FlowGroup(UInt32 id,Peer& peer,ServerHandler& serverHandler,BandWriter& band) : Flow(id,s_signature,s_name,peer,serverHandler,band),_pGroup(NULL) {
}

FlowGroup::~FlowGroup() {
	
}

void FlowGroup::complete() {
	// delete member of group
	DEBUG("Group closed")
	if(_pGroup)
		_pGroup->removePeer(peer);
	Flow::complete();
}


void FlowGroup::rawHandler(UInt8 type,PacketReader& data) {

	if(type==0x01) {
		if(data.available()>0) {
			UInt32 size = data.read7BitValue();

			vector<UInt8> groupId(size);
			data.readRaw(&groupId[0],size);

			_pGroup = &serverHandler.group(groupId);

			_pGroup->bestPeers(_bestPeers,peer);

			_pGroup->addPeer(peer);
			if(_bestPeers.empty())
				return;

			while(!_bestPeers.empty()) {
				BinaryWriter& response(writer.writeRawMessage(true));
			
				response.write8(0x0b); // unknown
				response.writeRaw(_bestPeers.front()->id,ID_SIZE);
				_bestPeers.pop_front();
			}
		}
	} else
		Flow::rawHandler(type,data);
}

} // namespace Cumulus
