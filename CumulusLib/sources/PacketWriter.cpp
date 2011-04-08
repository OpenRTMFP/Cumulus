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

#include "PacketWriter.h"
#include "Logs.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

PacketWriter::PacketWriter(const UInt8* buffer,int size) : _memory((char*)buffer,size),BinaryWriter(_memory),_pOther(NULL),_skip(0),_size(size) {
}

// Consctruction by copy
PacketWriter::PacketWriter(PacketWriter& other,int skip) : _pOther(&other),_memory(other._memory),BinaryWriter(_memory),_skip(skip),_size(other._size) {
	this->next(skip);
}

PacketWriter::~PacketWriter() {
	flush();
}

void PacketWriter::limit(int length) {
	if(length<=0)
		length = _size;
	if(length>_size) {
		WARN("Limit '%d' more upper than buffer size '%d' bytes",length,_size);
		length = _size;
	}
	_memory.resize(length);
}

void PacketWriter::clear(int pos) {
	reset(pos);
	_memory.written(pos);
}

void PacketWriter::flush() {
	if(_pOther && (_memory.written()-_skip)>_pOther->_memory.written())
		_pOther->_memory.written(_memory.written());
	BinaryWriter::flush();
}


} // namespace Cumulus
