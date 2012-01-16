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
#include "Flow.h"


using namespace std;
using namespace Poco;
using namespace Poco::Util;

namespace Cumulus {


AMFWriter::AMFWriter(BinaryWriter& writer) : writer(writer),_amf3(false),amf0Preference(false),lastReference(0) {

}

AMFWriter::~AMFWriter() {

}

bool AMFWriter::repeat(UInt32 reference) {
	if(reference==0 || reference>_references.size()) {
		ERROR("No AMF reference to repeat");
		return false;
	}
	--reference;
	if(!_amf3)
		writer.write8(AMF_AVMPLUS_OBJECT);
	writer.write8(_references[reference]);
	writer.write8(reference<<1);
	return true;
}

void AMFWriter::write(const string& value) {
	(UInt32&)lastReference=0;
	if(value.empty()) {
		writer.write8(_amf3 ? 0x01 : AMF_UNDEFINED);
		return;
	}
	if(!_amf3) {
		if(amf0Preference) {
			if(value.size()>65535) {
				writer.write8(AMF_LONG_STRING);
				writer.write32(value.size());
				writer.writeRaw(value);
			} else {
				writer.write8(AMF_STRING);
				writer.writeString16(value);
			}
			return;
		}
		writer.write8(AMF_AVMPLUS_OBJECT);
	}
	writer.write8(AMF3_STRING); // marker
	writeString(value);
	(UInt32&)lastReference=_references.size();
}

void AMFWriter::writePropertyName(const string& value) {
	(UInt32&)lastReference=0;
	// no marker, no string_long, no empty value
	if(!_amf3) {
		writer.writeString16(value);
		return;
	}
	writeString(value);
}

void AMFWriter::writeString(const string& value) {
	if(!value.empty()) {
		map<string,UInt32>::iterator it = _stringReferences.lower_bound(value);
		if(it!=_stringReferences.end() && it->first==value) {
			writer.write7BitValue(it->second<<1);
			return;
		}
		if(it!=_stringReferences.begin())
			--it;
		_stringReferences.insert(it,pair<string,UInt32>(value,_stringReferences.size()));
	}
	writer.write7BitValue((value.size()<<1) | 0x01);
	writer.writeRaw(value);
}

void AMFWriter::writeNull() {
	(UInt32&)lastReference=0;
	writer.write8(_amf3 ? AMF3_NULL : AMF_NULL); // marker
}

void AMFWriter::writeBoolean(bool value){
	(UInt32&)lastReference=0;
	if(!_amf3) {
		writer.write8(AMF_BOOLEAN); // marker
		writer << value;
	} else
		writer.write8(value ? AMF3_TRUE : AMF3_FALSE);
}

void AMFWriter::writeDate(const Timestamp& date){
	(UInt32&)lastReference=0;
	if(!_amf3) {
		if(amf0Preference) {
			writer.write8(AMF_DATE);
			writer << ((double)date.epochMicroseconds()/1000);
			writer.write16(0); // Timezone, TODO?
			return;
		}
		writer.write8(AMF_AVMPLUS_OBJECT);
	}
	writer.write8(AMF3_DATE);
	writer.write8(0x01);
	writer << ((double)date.epochMicroseconds()/1000);
	_references.push_back(AMF3_DATE);
	(UInt32&)lastReference=_references.size();
}

void AMFWriter::writeNumber(double value){
	(UInt32&)lastReference=0;
	writer.write8(_amf3 ? AMF3_NUMBER : AMF_NUMBER); // marker
	writer << value;
}

void AMFWriter::writeInteger(Int32 value){
	(UInt32&)lastReference=0;
	if(!_amf3) {
		if(amf0Preference) {
			writeNumber((double)value);
			return;
		}
		writer.write8(AMF_AVMPLUS_OBJECT);
	}
	writer.write8(AMF3_INTEGER); // marker
	if(value>268435455) {
		ERROR("AMF Integer maximum value reached");
		value=268435455;
	} else if(value<0)
		value+=(1<<29);
	writer.write7BitValue(value);
}

BinaryWriter& AMFWriter::writeByteArray(UInt32 size) {
	(UInt32&)lastReference=0;
	if(!_amf3)
		writer.write8(AMF_AVMPLUS_OBJECT); // switch in AMF3 format 
	writer.write8(AMF3_BYTEARRAY); // bytearray in AMF3 format!
	writer.write7BitValue((size << 1) | 1);
	_references.push_back(AMF3_BYTEARRAY);
	(UInt32&)lastReference=_references.size();
	return size==0 ? BinaryWriter::BinaryWriterNull : writer;
}

void AMFWriter::writeByteArray(const UInt8* data,UInt32 size) {
	BinaryWriter& writer = writeByteArray(size);
	writer.writeRaw(data,size);
}

void AMFWriter::beginDictionary(UInt32 count,bool weakKeys) {
	(UInt32&)lastReference=0;
	if(!_amf3) {
		writer.write8(AMF_AVMPLUS_OBJECT);
		_amf3=true;
	}
	writer.write8(AMF3_DICTIONARY);
	writer.write7BitValue((count << 1) | 1);
	writer.write8(weakKeys ? 0x01 : 0x00);
	_references.push_back(AMF3_DICTIONARY);
	(UInt32&)lastReference=_references.size();
	_lastObjectReferences.push_back(lastReference);
}

void AMFWriter::endDictionary() {
	if(_lastObjectReferences.size()==0) {
		ERROR("AMFWriter::beginDictionary called without endDictionary calling");
		return;
	}
	(UInt32&)lastReference=_lastObjectReferences.back();
	_lastObjectReferences.pop_back();
	if(_lastObjectReferences.size()==0)
		_amf3=false;
}

void AMFWriter::beginObjectArray(UInt32 count) {
	(UInt32&)lastReference=0;
	if(!_amf3) {
		writer.write8(AMF_AVMPLUS_OBJECT);
		_amf3=true;
	}
	writer.write8(AMF3_ARRAY);
	writer.write7BitValue((count << 1) | 1);
	_references.push_back(AMF3_ARRAY);
	(UInt32&)lastReference=_references.size();
	_lastObjectReferences.push_back(lastReference);
	_lastObjectReferences.push_back(lastReference);
}

void AMFWriter::beginArray(UInt32 count) {
	beginObjectArray(count);
	endObject();
}

void AMFWriter::endArray() {
	if(_lastObjectReferences.size()==0) {
		ERROR("AMFWriter::endArray called without beginArray calling");
		return;
	}
	(UInt32&)lastReference=_lastObjectReferences.back();
	_lastObjectReferences.pop_back();
	if(_lastObjectReferences.size()==0)
		_amf3=false;
}

BinaryWriter& AMFWriter::beginExternalizableObject(const std::string& type) {
	beginObject(type,true);
	return writer;
}

void AMFWriter::endExternalizableObject() {
	if(_lastObjectReferences.size()==0) {
		ERROR("AMFWriter::endArray called without beginArray calling");
		return;
	}
	(UInt32&)lastReference=_lastObjectReferences.back();
	_lastObjectReferences.pop_back();
	if(_lastObjectReferences.size()==0)
		_amf3=false;
}


void AMFWriter::beginObject(const string& type,bool externalizable) {
	(UInt32&)lastReference=0;
	if(!_amf3) {
		if(amf0Preference && !externalizable) {
			_lastObjectReferences.push_back(0);
			if(type.empty())
				writer.write8(AMF_BEGIN_OBJECT);
			else {
				writer.write8(AMF_BEGIN_TYPED_OBJECT);
				write(type);
			}
			return;
		}
		writer.write8(AMF_AVMPLUS_OBJECT);
		_amf3=true;
	}
	writer.write8(AMF3_OBJECT);
	_references.push_back(AMF3_OBJECT);
	(UInt32&)lastReference=_references.size();
	_lastObjectReferences.push_back(lastReference);


	UInt32 flags = 1; // inner object
	
	// ClassDef always inline (because never hard properties, all is dynamic)
	// _references.push_back(0);
	flags|=(1<<1);
	if(externalizable)
		flags|=(1<<2);
	else
		flags|=(1<<3); // Always dynamic, but can't be externalizable AND dynamic!


	/* TODO?
	if(externalizable) {
		// What follows is the value of the “inner” object
	} else if(hardProperties>0 && !pClassDef) {
		// The remaining integer-data represents the number of class members that exist.
		// If there is a class-def reference there are no property names and the number of values is equal to the number of properties in the class-def
		flags |= (hardProperties<<4);
	}*/

	writer.write7BitValue(flags);
	writePropertyName(type);
}

void AMFWriter::endObject() {
	if(_lastObjectReferences.size()==0) {
		ERROR("AMFWriter::endObject called without beginObject calling");
		return;
	}
	(UInt32&)lastReference=_lastObjectReferences.back();
	_lastObjectReferences.pop_back();
	if(!_amf3) {
		writer.write16(0); 
		writer.write8(AMF_END_OBJECT);
		return;
	}
	writer.write8(01); // end marker
	if(_lastObjectReferences.size()==0)
		_amf3=false;
}

void AMFWriter::writeObjectProperty(const string& name,const Timestamp& date) {
	writePropertyName(name);
	writeDate(date);
}

void AMFWriter::writeObjectProperty(const string& name,double value) {
	writePropertyName(name);
	writeNumber(value);
}

void AMFWriter::writeObjectProperty(const string& name,Int32 value) {
	writePropertyName(name);
	writeInteger(value);
}

void AMFWriter::writeObjectProperty(const string& name,const vector<UInt8>& data) {
	writePropertyName(name);
	writeByteArray(data);
}

void AMFWriter::writeObjectProperty(const string& name,const string& value) {
	writePropertyName(name);
	write(value);
}

void AMFWriter::writeObjectProperty(const string& name) {
	writePropertyName(name);
	writeNull();
}


void AMFWriter::writeSimpleObject(const AMFSimpleObject& object) {
	beginObject();
	AbstractConfiguration::Keys keys;
	object.keys(keys);
	AbstractConfiguration::Keys::const_iterator it;
	for(it=keys.begin();it!=keys.end();++it) {
		string name = *it;
		int type = object.getInt(name+".type",-1);
		switch(type) {
			case AMF::Boolean:
				writeObjectProperty(name,object.getBool(name));
				break;
			case AMF::String:
				writeObjectProperty(name,object.getString(name));
				break;
			case AMF::Number:
				writeObjectProperty(name,object.getDouble(name));
				break;
			case AMF::Integer:
				writeObjectProperty(name,object.getInt(name));
				break;
			case AMF::Date: {
				Timestamp date((Timestamp::TimeVal)object.getDouble(name)*1000);
				writeObjectProperty(name,date);
				break;
			}
			case AMF::Null:
				writeObjectProperty(name);
				break;
			default:
				ERROR("Unknown AMFObject %d type",type);
		}
	}
	endObject();
}




} // namespace Cumulus
