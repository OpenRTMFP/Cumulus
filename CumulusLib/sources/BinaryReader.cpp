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

using namespace std;
using namespace Poco;

namespace Cumulus {

BinaryReader::BinaryReader(istream& istr) : Poco::BinaryReader(istr,BinaryReader::NETWORK_BYTE_ORDER) {
}


BinaryReader::~BinaryReader() {
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
	UInt8 a=0,b=0,c=0,d=0;
	Int8 s = 0;
	*this >> a;
	if(a & 0x80) {
		*this >> b;++s;
		if(b & 0x80) {
			*this >> c;++s;
			if(c & 0x80) {
				*this >> d;++s;
			}
		}
	}
	UInt32 value = ((a&0x7F)<<(s*7));
	--s;
	if(s<0)
		return value;
	value += ((b&0x7F)<<(s*7));
	--s;
	if(s<0)
		return value;
	value += ((c&0x7F)<<(s*7));
	--s;
	if(s<0)
		return value;
	return value + ((d&0x7F)<<(s*7));
}


} // namespace Cumulus
