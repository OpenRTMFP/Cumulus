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
#include <openssl/evp.h>
#include <math.h>

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

string FlowGroup::Signature("\x00\x47\x43",3);
string FlowGroup::_Name("NetGroup");

FlowGroup::FlowGroup(UInt32 id,Peer& peer,Handler& handler,BandWriter& band) : Flow(id,Signature,_Name,peer,handler,band),_pGroup(NULL) {
	(UInt32&)writer.flowId = id;
}

FlowGroup::~FlowGroup() {
	// delete member of group
	DEBUG("Group closed")
	if(_pGroup)
		_pGroup->removePeer(peer);
}

void FlowGroup::rawHandler(UInt8 type,PacketReader& data) {

	if(type==0x01) {
		if(data.available()>0) {
			UInt32 size = data.read7BitValue()-1;
			UInt8 flag = data.read8();

			UInt8 groupId[ID_SIZE];

			if(flag==0x10) {
				vector<UInt8> groupIdVar(size);
				data.readRaw(&groupIdVar[0],size);
				EVP_Digest(&groupIdVar[0],groupIdVar.size(),(unsigned char *)groupId,NULL,EVP_sha256(),NULL);
			} else
				data.readRaw(groupId,ID_SIZE);
		
			_pGroup = &handler.group(groupId);
		
			UInt16 count=13;
			if(_pGroup->peers().size()>600)
				count=(UInt16)floor(2*log((double)_pGroup->peers().size()));

			map<UInt32,const Peer*>::const_iterator it;
			for(it=_pGroup->peers().begin();it!=_pGroup->peers().end();++it) {
				if((*it->second)==peer)
					continue;
				BinaryWriter& response(writer.writeRawMessage(true));
				response.write8(0x0b); // unknown
				response.writeRaw(it->second->id,ID_SIZE);
				if(--count==0)
					break;
			}

			_pGroup->addPeer(peer);

		}
	} else
		Flow::rawHandler(type,data);
}

} // namespace Cumulus
