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

#pragma once

#include "Cumulus.h"
#include "PacketReader.h"
#include "BinaryWriter.h"

namespace Cumulus {

class Listener {
public:
	Listener();
	virtual ~Listener();

	void pushRawPacket(Poco::UInt8 type,PacketReader& packet);
	void pushAudioPacket(PacketReader& packet); 
	void pushVideoPacket(PacketReader& packet);
	
private:
	virtual void flush()=0;
	virtual BinaryWriter& writer()=0;
};

inline void Listener::pushAudioPacket(PacketReader& packet) {
	pushRawPacket(0x08,packet);
}

inline void Listener::pushVideoPacket(PacketReader& packet) {
	pushRawPacket(0x09,packet);
}


} // namespace Cumulus
