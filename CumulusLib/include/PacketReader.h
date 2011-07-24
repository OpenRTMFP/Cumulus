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
#include "MemoryStream.h"
#include "BinaryReader.h"

#define PACKETRECV_SIZE		2048

namespace Cumulus {


class PacketReader: public BinaryReader {
public:
	PacketReader(const Poco::UInt8* buffer,Poco::UInt32 size);
	PacketReader(PacketReader&);
	virtual ~PacketReader();

	const Poco::UInt32	fragments;

	Poco::UInt32	available();
	Poco::UInt8*	current();
	Poco::UInt32	position();

	void			reset(Poco::UInt32 newPos=0);
	void			shrink(Poco::UInt32 rest);
	void			next(Poco::UInt32 size);
private:
	MemoryInputStream _memory;
	
};

inline Poco::UInt32 PacketReader::available() {
	return _memory.available();
}

inline Poco::UInt32 PacketReader::position() {
	return _memory.current()-_memory.begin();
}

inline void PacketReader::reset(Poco::UInt32 newPos) {
	_memory.reset(newPos);
}

inline void PacketReader::next(Poco::UInt32 size) {
	return _memory.next(size);
}

inline Poco::UInt8* PacketReader::current() {
	return (Poco::UInt8*)_memory.current();
}


} // namespace Cumulus
