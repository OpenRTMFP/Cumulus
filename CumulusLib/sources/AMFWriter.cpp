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

#include "AMFWriter.h"
#include "Logs.h"

using namespace std;
using namespace Poco;
using namespace Poco::Util;

namespace Cumulus {

AMFWriter::AMFWriter(BinaryWriter& writer) : writer(writer) {

}

AMFWriter::~AMFWriter() {

}

void AMFWriter::writeArray(UInt32 count) {
	writer.write8(AMF_STRICT_ARRAY);
	writer.write32(count);
}

void AMFWriter::writeResponseHeader(const string& key,double callbackHandle) {
	writer.write8(0x14);
	writer.write32(0);
	write(key);
	writeNumber(callbackHandle);
	writeNull();
}

void AMFWriter::writeBool(bool value){
	writer.write8(AMF_BOOLEAN); // marker
	writer << value;
}

void AMFWriter::write(const char* value,UInt16 size) {
	if(size==0) {
		writer.write8(AMF_UNDEFINED);
		return;
	}
	writer.write8(AMF_STRING); // marker
	writer.writeString16(value,size);
}
void AMFWriter::write(const string& value) {
	if(value.empty()) {
		writer.write8(AMF_UNDEFINED);
		return;
	}
	writer.write8(AMF_STRING); // marker
	writer.writeString16(value);
}

void AMFWriter::writeNumber(double value){
	writer.write8(AMF_NUMBER); // marker
	writer << value;
}

void AMFWriter::writeObject(const AMFObject& amfObject) {
	beginObject();
	AbstractConfiguration::Keys keys;
	amfObject.keys(keys);
	AbstractConfiguration::Keys::const_iterator it;
	for(it=keys.begin();it!=keys.end();++it) {
		string name = *it;
		writer.writeString16(name);
		int type = amfObject.getInt(name+".type",-1);
		switch(type) {
			case AMF_BOOLEAN:
				writeBool(amfObject.getBool(name));
				break;
			case AMF_STRING:
				write(amfObject.getString(name));
				break;
			case AMF_NUMBER:
				writeNumber(amfObject.getDouble(name));
				break;
			case AMF_UNDEFINED:
				write("");
				break;
			case AMF_NULL:
				writeNull();
				break;
			default:
				ERROR("Unknown AMF '%d' type",type);
		}
	}
	endObject();
}

void AMFWriter::writeObjectArrayProperty(const string& name,UInt32 count) {
	writer.writeString16(name);
	writeArray(count);
}

void AMFWriter::beginSubObject(const string& name) {
	writer.writeString16(name);
	beginObject();
}

void AMFWriter::writeObjectProperty(const string& name,double value) {
	writer.writeString16(name);
	writeNumber(value);
}

void AMFWriter::writeObjectProperty(const string& name,const vector<UInt8>& data) {
	writer.writeString16(name);
	writeByteArray(data);
}

void AMFWriter::writeObjectProperty(const string& name,const string& value) {
	writer.writeString16(name);
	write(value);
}

void AMFWriter::writeObjectProperty(const string& name,const char* value,UInt16 size) {
	writer.writeString16(name);
	write(value,size);
}

BinaryWriter& AMFWriter::writeByteArray(UInt32 size) {
	if(size==0) {
		writer.write8(AMF_UNDEFINED);
		return writer;
	}
	writer.write8(AMF_AVMPLUS_OBJECT); // switch in AMF3 format 
	writer.write8(AMF_LONG_STRING); // bytearray in AMF3 format!
	writer.write7BitValue((size << 1) | 1);
	return writer;
}

void AMFWriter::writeByteArray(const UInt8* data,UInt32 size) {
	BinaryWriter& writer = writeByteArray(size);
	writer.writeRaw(data,size);
}

void AMFWriter::endObject() {
	// mark end
	writer.write16(0); 
	writer.write8(AMF_END_OBJECT);
}



} // namespace Cumulus
