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

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

string FlowNull::s_name;
string FlowNull::s_signature;

FlowNull::FlowNull(Peer& peer,ServerHandler& serverHandler,BandWriter& band) : Flow(0,s_signature,s_name,peer,serverHandler,band) {
}

FlowNull::~FlowNull() {
}

void FlowNull::messageHandler(UInt32 stage,PacketReader& message,UInt8 flags) {
	Flow::messageHandler(stage,message,flags);
	fail("Flow unknown certainly already consumed");
}
void FlowNull::rawHandler(UInt8 type,PacketReader& data) {
	Flow::rawHandler(type,data);
	fail("Flow unknown certainly already consumed");
}
void FlowNull::audioHandler(PacketReader& packet) {
	Flow::audioHandler(packet);
	fail("Flow unknown certainly already consumed");
}
void FlowNull::videoHandler(PacketReader& packet) {
	Flow::videoHandler(packet);
	fail("Flow unknown certainly already consumed");
}

} // namespace Cumulus
