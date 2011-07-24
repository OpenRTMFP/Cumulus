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

#include "FlowNull.h"
#include "FlowWriter.h"
#include "Poco/Format.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

string FlowNull::_Name;
string FlowNull::_Signature;

FlowNull::FlowNull(Peer& peer,Handler& handler,BandWriter& band) : Flow(0,_Signature,_Name,peer,handler,band) {
	
}

FlowNull::~FlowNull() {
}

void FlowNull::fragmentHandler(UInt32 stage,UInt32 deltaNAck,PacketReader& fragment,UInt8 flags) {
	fail("Message received for a Flow unknown");
	(UInt32&)this->stage = stage;
}


} // namespace Cumulus
