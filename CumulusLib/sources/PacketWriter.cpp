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

#include "PacketWriter.h"
#include "Logs.h"
#include "Poco/RandomStream.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

PacketWriter::PacketWriter(int pos) : _memory((char*)_buff,MAX_SIZE_MSG),BinaryWriter(_memory,BinaryWriter::NETWORK_BYTE_ORDER),_pOther(NULL) {
	reset(pos);
}

// Consctruction by copy
PacketWriter::PacketWriter(PacketWriter& other) : _pOther(&other),_memory(other._memory),BinaryWriter(_memory,BinaryWriter::NETWORK_BYTE_ORDER) {
}

PacketWriter::~PacketWriter() {
	flush();
}

void PacketWriter::writeString8(const char* value,UInt8 size) {
	write8(size);
	writeRaw(value);
}
void PacketWriter::writeString16(const char* value,UInt16 size) {
	write16(size);
	writeRaw(value);
}
void PacketWriter::writeString8(const string& value) {
	write8(value.size());
	writeRaw(value);
}
void PacketWriter::writeString16(const string& value) {
	write16(value.size());
	writeRaw(value);
}

void PacketWriter::clear(int pos) {
	reset(pos);
	_memory.written(pos);
}

void PacketWriter::flush() {
	if(_pOther && _memory.written()>_pOther->_memory.written())
		_pOther->_memory.written(_memory.written());
	BinaryWriter::flush();
}

void PacketWriter::writeRandom(UInt16 size) {
	char * value = new char[size]();
	RandomInputStream().read(value,size);
	writeRaw(value,size);
	delete [] value;
}



} // namespace Cumulus
