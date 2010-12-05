/* 
	Copyright 2010 cumulus.dev@gmail.com
 
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
#include "PacketWriter.h"
#include "Poco/BinaryReader.h"

namespace Cumulus {


class PacketReader: public Poco::BinaryReader {
public:
	PacketReader(Poco::UInt8* packet,int size);
	PacketReader(PacketReader&);
	PacketReader(PacketWriter&);
	virtual ~PacketReader();

	void readString8(std::string& value);
	Poco::UInt8		next8();
	Poco::UInt16	next16();
	Poco::UInt32	next32();

	int				available();
	Poco::UInt8*	current();
	int				position();

	void			reset(int newPos=0,int newSize=0);
	void			skip(int size);
private:
	MemoryInputStream _memory;
	
};

inline void PacketReader::readString8(std::string& value) {
	readRaw(next8(),value);
}

inline int PacketReader::available() {
	return _memory.available();
}

inline int PacketReader::position() {
	return _memory.current()-_memory.begin();
}

inline void PacketReader::reset(int newPos,int newSize) {
	_memory.reset(newPos,newSize);
}

inline void PacketReader::skip(int size) {
	return _memory.skip(size);
}

inline Poco::UInt8* PacketReader::current() {
	return (Poco::UInt8*)_memory.current();
}


} // namespace Cumulus
