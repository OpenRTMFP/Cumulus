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

#include "BinaryWriter.h"
#include "Poco/RandomStream.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

BinaryWriter::BinaryWriter(std::ostream& ostr) : Poco::BinaryWriter(ostr,BinaryWriter::NETWORK_BYTE_ORDER) {
}


BinaryWriter::~BinaryWriter() {
	flush();
}

void BinaryWriter::writeString(const string& value) {
	write7BitValue(value.size());
	writeRaw(value);
}

void BinaryWriter::writeString8(const char* value,UInt8 size) {
	write8(size);
	writeRaw(value,size);
}
void BinaryWriter::writeString8(const string& value) {
	write8(value.size());
	writeRaw(value);
}
void BinaryWriter::writeString16(const char* value,UInt16 size) {
	write16(size);
	writeRaw(value,size);
}
void BinaryWriter::writeString16(const string& value) {
	write16(value.size());
	writeRaw(value);
}

void BinaryWriter::writeRandom(UInt16 size) {
	char * value = new char[size]();
	RandomInputStream().read(value,size);
	writeRaw(value,size);
	delete [] value;
}

void BinaryWriter::writeAddress(const Address& address,bool publicFlag) {
	UInt8 flag = publicFlag ? 0x02 : 0x01;
	if(address.host.size()==16) // IPv6
		flag |= 0x80;
	write8(flag);
	for(int i=0;i<address.host.size();++i)
		write8(address.host[i]);
	write16(address.port);
}

void BinaryWriter::writeAddress(const SocketAddress& address,bool publicFlag) {
	UInt8 flag = publicFlag ? 0x02 : 0x01;
	UInt8 size = 4;
	IPAddress host = address.host();
	if(host.family() == IPAddress::IPv6) {
		flag |= 0x80;
		size = 16;
	}
	const UInt8* bytes = reinterpret_cast<const UInt8*>(host.addr());
	write8(flag);
	for(int i=0;i<size;++i)
		write8(bytes[i]);
	write16(address.port());
}


void BinaryWriter::write7BitValue(UInt32 value) {
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
