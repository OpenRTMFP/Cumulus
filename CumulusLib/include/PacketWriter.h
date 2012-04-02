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
#include "BinaryWriter.h"

#define PACKETSEND_SIZE			1215

namespace Cumulus {

class PacketWriter: public BinaryWriter {
public:
	PacketWriter(const Poco::UInt8* buffer,Poco::UInt32 size);
	PacketWriter(PacketWriter&);
	virtual ~PacketWriter();

	Poco::UInt8*		begin();
	Poco::UInt32		length();
	Poco::UInt32		position();
	
	Poco::UInt32		available();

	bool	good();
	void	clear(Poco::UInt32 pos=0);
	void	reset(Poco::UInt32 newPos);
	void	limit(Poco::UInt32 length=0);
	void	next(Poco::UInt32 size);
	void	flush();
	void	clip(Poco::UInt32 offset);

private:
	MemoryOutputStream	_memory;
	PacketWriter*		_pOther;
	Poco::UInt32		_size;
};

inline void PacketWriter::clip(Poco::UInt32 offset) {
	_memory.clip(offset);
}
inline Poco::UInt32 PacketWriter::available() {
	return _memory.available();
}
inline bool PacketWriter::good() {
	return _memory.good();
}
inline Poco::UInt32 PacketWriter::length() {
	return _memory.written();
}
inline Poco::UInt32 PacketWriter::position() {
	return _memory.current()-(char*)begin();
}
inline void PacketWriter::reset(Poco::UInt32 newPos) {
	_memory.reset(newPos);
}
inline void PacketWriter::next(Poco::UInt32 size) {
	return _memory.next(size);
}

inline Poco::UInt8* PacketWriter::begin() {
	return (Poco::UInt8*)_memory.begin();
}


} // namespace Cumulus
