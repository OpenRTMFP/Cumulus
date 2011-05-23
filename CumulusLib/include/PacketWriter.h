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
	PacketWriter(const Poco::UInt8* buffer,std::streamsize size);
	PacketWriter(PacketWriter&,std::streamsize skip=0);
	virtual ~PacketWriter();

	Poco::UInt8*		begin();
	std::streamsize		length();
	int					position();
	
	std::streamsize		available();

	bool	good();
	void	clear(std::streampos pos=0);
	void	reset(std::streampos newPos=-1);
	void	limit(std::streamsize length=0);
	void	clip(std::streampos offset);
	void	next(std::streamsize size);
	void	flush();
	
private:
	std::streamsize		_skip;
	MemoryOutputStream	_memory;
	PacketWriter*		_pOther;
	std::streamsize		_size;
};

inline std::streamsize PacketWriter::available() {
	return _memory.available();
}
inline bool PacketWriter::good() {
	return _memory.good();
}
inline std::streamsize PacketWriter::length() {
	return _memory.written();
}
inline int PacketWriter::position() {
	return _memory.current()-(char*)begin();
}
inline void PacketWriter::reset(std::streampos newPos) {
	_memory.reset(newPos);
}
inline void PacketWriter::clip(std::streampos offset) {
	_memory.clip(offset);
}
inline void PacketWriter::next(std::streamsize size) {
	return _memory.next(size);
}

inline Poco::UInt8* PacketWriter::begin() {
	return (Poco::UInt8*)_memory.begin();
}


} // namespace Cumulus
