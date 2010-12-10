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

#include "AMFReader.h"
#include "Logs.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

AMFReader::AMFReader(PacketReader& reader) : _reader(reader) {

}


AMFReader::~AMFReader() {

}

void AMFReader::read(string& value) {
	UInt8 c = _reader.next8();
	if(c!=0x02) {
		ERROR("byte '%02x' is not a AMF String marker",c);
		return;
	}
	_reader.readString16(value);
}

double AMFReader::readNumber() {
	UInt8 c = _reader.next8();
	if(c!=0x00) {
		ERROR("byte '%02x' is not a AMF number marker",c);
		return 0;
	}
	double result;
	_reader >> result;
	return result;
}


void AMFReader::readNull() {
	UInt8 c = _reader.next8();
	if(c!=0x05)
		ERROR("byte '%02x' is not a AMF Null marker",c);
}


} // namespace Cumulus
