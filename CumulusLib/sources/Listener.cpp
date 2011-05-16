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

#include "Listener.h"
#include "Poco/StreamCopier.h"

using namespace Poco;
using namespace std;

namespace Cumulus {

Listener::Listener(UInt32 flowId,const string& signature,BandWriter& band) : FlowWriter(flowId,signature,band) {
	/*BinaryWriter& data = writeRawMessage(); TODO added it? useful?
	data.write16(0x22);
	data.write32(0);
	data.write32(0x02);*/
}

Listener::~Listener() {
}

void Listener::pushRawPacket(UInt8 type,PacketReader& packet) {
	BinaryWriter& data = writeRawMessage(true);
	data.write8(type);
	if(type==0x04)
		data.write32(0);
	StreamCopier::copyStream(packet.stream(),data.stream());
	flush();
}

void Listener::pushVideoPacket(PacketReader& packet) {
	BinaryWriter& data = writeRawMessage(true);
	data.write8(0x09);
	UInt32 elapsed = (UInt32)(_time.elapsed()/1000);
	data.write32(elapsed);
	StreamCopier::copyStream(packet.stream(),data.stream());
	flush();
}

void Listener::pushAudioPacket(PacketReader& packet) {
	BinaryWriter& data = writeRawMessage(true);
	data.write8(0x08);
	UInt32 elapsed = (UInt32)(_time.elapsed()/1000);
	data.write32(elapsed);
	StreamCopier::copyStream(packet.stream(),data.stream());
	flush();
}


} // namespace Cumulus
