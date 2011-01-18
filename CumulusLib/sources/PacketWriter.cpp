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
#include "Poco/RandomStream.h"
#include "Poco/Net/IPAddress.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

PacketWriter::PacketWriter(const UInt8* buffer,int size,int skip) : _memory((char*)buffer,size),BinaryWriter(_memory,BinaryWriter::NETWORK_BYTE_ORDER),_pOther(NULL),_skip(skip) {
	this->next(skip);
}

// Consctruction by copy
PacketWriter::PacketWriter(PacketWriter& other,int skip) : _pOther(&other),_memory(other._memory),BinaryWriter(_memory,BinaryWriter::NETWORK_BYTE_ORDER),_skip(skip) {
	this->next(skip);
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
	if(_pOther && (_memory.written()-_skip)>_pOther->_memory.written())
		_pOther->_memory.written(_memory.written());
	BinaryWriter::flush();
}

void PacketWriter::writeRandom(UInt16 size) {
	char * value = new char[size]();
	RandomInputStream().read(value,size);
	writeRaw(value,size);
	delete [] value;
}

void PacketWriter::writeAddress(const SocketAddress& address,bool publicFlag) {
	UInt8 flag = publicFlag ? 0x02 : 0x01;
	UInt8 i;
	IPAddress host = address.host();
	if(host.family() == IPAddress::IPv6) {
		write8(flag&0x80);
		const UInt16* words = reinterpret_cast<const UInt16*>(host.addr());
		for(i=0;i<4;++i)
			write16(words[i]);
	} else {
		write8(flag);
		const UInt8* bytes = reinterpret_cast<const UInt8*>(host.addr());
		for(i=0;i<4;++i)
			write8(bytes[i]);
	}
	write16(address.port());
}


} // namespace Cumulus
