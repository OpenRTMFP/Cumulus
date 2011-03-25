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

#include "FlowStream.h"
#include "Logs.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;


namespace Cumulus {

string FlowStream::s_signature("\x00\x54\x43\x04\x01",5);
string FlowStream::s_name("NetStream");

FlowStream::FlowStream(Peer& peer,ServerHandler& serverHandler) : Flow(s_name,peer,serverHandler) {
}

FlowStream::~FlowStream() {
}


bool FlowStream::rawHandler(Poco::UInt8 stage,PacketReader& request,ResponseWriter& responseWriter) {

	return false;
}

} // namespace Cumulus
