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
#include "Poco/format.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

string FlowNull::s_name;
string FlowNull::s_signature;

FlowNull::FlowNull(Peer& peer,ServerHandler& serverHandler,BandWriter& band) : Flow(0,s_signature,s_name,peer,serverHandler,band) {
	writer().close();
}

FlowNull::~FlowNull() {
}

void FlowNull::fragmentHandler(UInt32 stage,UInt32 deltaNAck,PacketReader& fragment,UInt8 flags) {
	fail(format("Message received for a Flow %u unknown",id));
	(UInt32&)this->stage = stage;
}

void FlowNull::commit() {
	(UInt32&)id=0;
	Flow::commit();
}

} // namespace Cumulus
