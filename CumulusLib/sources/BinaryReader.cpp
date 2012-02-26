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

#include "BinaryReader.h"
#include "Util.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

BinaryReader BinaryReader::BinaryReaderNull(Util::NullInputStream);

BinaryReader::BinaryReader(istream& istr) : Poco::BinaryReader(istr,BinaryReader::NETWORK_BYTE_ORDER) {
}


BinaryReader::~BinaryReader() {
}

UInt32 BinaryReader::read7BitEncoded() {
	UInt32 id;
	Poco::BinaryReader::read7BitEncoded(id);
	return id;
}

UInt8 BinaryReader::read8() {
	UInt8 c;
	(*this) >> c;
	return c;
}

UInt16 BinaryReader::read16() {
	UInt16 c;
	(*this) >> c;
	return c;
}

UInt32 BinaryReader::read32() {
	UInt32 c;
	(*this) >> c;
	return c;
}

UInt32 BinaryReader::read7BitValue() {
	UInt8 n = 0;
    UInt8 b = read8();
    UInt32 result = 0;
    while ((b&0x80) && n < 3) {
        result <<= 7;
        result |= (b&0x7F);
        b = read8();
        ++n;
    }
    result <<= ((n<3) ? 7 : 8); // Use all 8 bits from the 4th byte
    result |= b;
	return result;
}

UInt64 BinaryReader::read7BitLongValue() {
	UInt8 n = 0;
    UInt8 b = read8();
    UInt64 result = 0;
    while ((b&0x80) && n < 8) {
        result <<= 7;
        result |= (b&0x7F);
        b = read8();
        ++n;
    }
    result <<= ((n<8) ? 7 : 8); // Use all 8 bits from the 4th byte
    result |= b;
	return result;
}

bool BinaryReader::readAddress(Address& address) {
	UInt8 flag = read8();
	((vector<UInt8>&)address.host).resize(4);
	if((flag&0x0F)==0x8F)
		((vector<UInt8>&)address.host).resize(16);
	for(int i=0;i<address.host.size();++i)
		((vector<UInt8>&)address.host)[i] = read8();
	(UInt16&)address.port = read16();
	return flag==0x02;
}


} // namespace Cumulus
