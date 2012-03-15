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

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

string FlowGroup::Signature("\x00\x47\x43",3);
string FlowGroup::_Name("NetGroup");

FlowGroup::FlowGroup(UInt64 id,Peer& peer,Invoker& invoker,BandWriter& band) : Flow(id,Signature,_Name,peer,invoker,band),_pGroup(NULL) {
	(UInt64&)writer.flowId = id;
}

FlowGroup::~FlowGroup() {
	// delete member of group
	DEBUG("Group closed")
	if(_pGroup)
		peer.unjoinGroup(*_pGroup);
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
		
			_pGroup = invoker.groups(groupId);
		
			if(_pGroup) {
				UInt16 count=6;
				Group::Iterator it;
				for(it=_pGroup->begin();it!=_pGroup->end();++it) {
					if(peer==(*it)->id)
						continue;
					BinaryWriter& response(writer.writeRawMessage(true));
					response.write8(0x0b); // unknown
					response.writeRaw((*it)->id,ID_SIZE);
					if((*it)->ping>=1000)
						continue;
					if(--count==0)
						break;
				}
				peer.joinGroup(*_pGroup);
			} else
				_pGroup = &peer.joinGroup(groupId);
		}
	} else
		Flow::rawHandler(type,data);
}

} // namespace Cumulus
