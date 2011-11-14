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

AMFReader::AMFReader(PacketReader& reader) : reader(reader) {

}


AMFReader::~AMFReader() {

}

void AMFReader::read(string& value) {
	UInt8 marker = reader.read8();
	if(marker==AMF_NULL || marker==AMF_UNDEFINED)
		return;
	if(marker!=AMF_STRING) {
		ERROR("byte %u is not a AMF String marker",marker);
		return;
	}
	reader.readString16(value);
}

double AMFReader::readNumber() {
	UInt8 marker = reader.read8();
	if(marker==AMF_NULL || marker==AMF_UNDEFINED)
		return 0;
	if(marker!=AMF_NUMBER) {
		ERROR("byte %u is not a AMF number marker",marker);
		return 0;
	}
	double result;
	reader >> result;
	return result;
}

bool AMFReader::readBool() {
	UInt8 marker = reader.read8();
	if(marker==AMF_NULL || marker==AMF_UNDEFINED)
		return false;
	if(marker!=AMF_BOOLEAN) {
		ERROR("byte %u is not a AMF boolean marker",marker);
		return false;
	}
	return reader.read8()==0x00 ? true : false;
}


void AMFReader::skipNull() {
	while(AMF_NULL == reader.read8() && reader.available());
	reader.reset(reader.position()-1);
}

void AMFReader::readObject(AMFObject& amfObject) {
	UInt8 marker = reader.read8();
	if(marker==AMF_NULL || marker==AMF_UNDEFINED)
		return;
	if(marker!=AMF_BEGIN_OBJECT) {
		ERROR("byte %u is not a AMF begin-object marker",marker);
		return;
	}
	string name;
	reader.readString16(name);
	while(!name.empty()) {
		marker = reader.read8();
		switch(marker) {
			case AMF_BOOLEAN: {
				bool value;
				reader >> value;
				amfObject.setBool(name,value);
				break;
			}
			case AMF_STRING: {
				string value;
				reader.readString16(value);
				amfObject.setString(name,value);
				break;
			}
			case AMF_NUMBER: {
				double value;
				reader >> value;
				amfObject.setDouble(name,value);
				break;
			}
			case AMF_UNDEFINED:
				amfObject.setString(name,"");
				break;
			default:
				ERROR("Unknown AMF %u marker",marker);
			case AMF_NULL:
				amfObject.setInt(name,0);
				amfObject.setInt(name+".type",AMF_NULL);
				break;
		}
		reader.readString16(name);
	}
	marker = reader.read8();
	if(marker!=AMF_END_OBJECT) {
		ERROR("byte %u is not a AMF end-object marker",marker);
		return;
	}
}


} // namespace Cumulus
