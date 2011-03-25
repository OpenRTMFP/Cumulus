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

PacketWriter::PacketWriter(const UInt8* buffer,int size) : _memory((char*)buffer,size),BinaryWriter(_memory,BinaryWriter::NETWORK_BYTE_ORDER),_pOther(NULL),_skip(0) {
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

void PacketWriter::writeAddress(const Address& address,bool publicFlag) {
	UInt8 flag = publicFlag ? 0x02 : 0x01;
	if(address.host.size()==16) // IPv6
		flag &= 0x80;
	write8(flag);
	for(int i=0;i<address.host.size();++i)
		write8(address.host[i]);
	write16(address.port);
}

void PacketWriter::writeAddress(const SocketAddress& address,bool publicFlag) {
	UInt8 flag = publicFlag ? 0x02 : 0x01;
	UInt8 size = 4;
	IPAddress host = address.host();
	if(host.family() == IPAddress::IPv6) {
		flag &= 0x80;
		size = 16;
	}
	const UInt8* bytes = reinterpret_cast<const UInt8*>(host.addr());
	write8(flag);
	for(int i=0;i<size;++i)
		write8(bytes[i]);
	write16(address.port());
}


void PacketWriter::write7BitValue(UInt32 value) {
	UInt8 d=value&0x7F;
	value>>=7;
	UInt8 c=value&0x7F;
	value>>=7;
	UInt8 b=value&0x7F;
	value>>=7;
	UInt8 a=value&0x7F;

	if(a>0) {
		write8(0x80 | a);
		write8(0x80 | b);
		write8(0x80 | c);
	} else if(b>0) {
		write8(0x80 | b);
		write8(0x80 | c);
	} else if(c>0)
		write8(0x80 | c);
	write8(d);
}


} // namespace Cumulus
