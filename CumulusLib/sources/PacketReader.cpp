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

#include "PacketReader.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

PacketReader::PacketReader(Poco::UInt8* packet,int size) : _memory((char*)packet,size),BinaryReader(_memory,BinaryReader::NETWORK_BYTE_ORDER) {
}


// Consctruction by copy
PacketReader::PacketReader(PacketReader& other) : _memory(other._memory),BinaryReader(_memory,BinaryReader::NETWORK_BYTE_ORDER) {
}

// Consctruction by copy
PacketReader::PacketReader(PacketWriter& writer) : _memory((char*)writer.begin(),writer.size()),BinaryReader(_memory,BinaryReader::NETWORK_BYTE_ORDER) {
}


PacketReader::~PacketReader() {
}


UInt8 PacketReader::next8() {
	UInt8 c;
	(*this) >> c;
	return c;
}

UInt16 PacketReader::next16() {
	UInt16 c;
	(*this) >> c;
	return c;
}

UInt32 PacketReader::next32() {
	UInt32 c;
	(*this) >> c;
	return c;
}



} // namespace Cumulus
