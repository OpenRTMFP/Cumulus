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
#include "Poco/NumberParser.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

class ObjectDef {
public:
	ObjectDef(UInt32 amf3,UInt8 arrayType=0) : amf3(amf3),reset(0),dynamic(false),externalizable(false),count(0),arrayType(arrayType) {}

	list<string>	hardProperties;
	UInt32			reset;
	bool			dynamic;
	bool			externalizable;
	UInt32			count;
	UInt8			arrayType;
	const UInt32	amf3;
};


AMFReader::AMFReader(PacketReader& reader) : reader(reader),_reset(0),_amf3(0),_amf0Reset(0),_referencing(true) {

}


AMFReader::~AMFReader() {
	list<ObjectDef*>::iterator it;
	for(it=_objectDefs.begin();it!=_objectDefs.end();++it)
		delete *it;
}


void AMFReader::reset() {
	if(_reset>0) {
		reader.reset(_reset);
		_reset=0;
	}
}

bool AMFReader::available() {
	reset();
	return reader.available()>0;
}

void AMFReader::readNull() {
	reset();
	AMF::Type type = followingType();
	if(type==AMF::Null) {
		reader.next(1);
		return;
	}
	ERROR("Type %.2x is not a AMF Null type",type);
}

double AMFReader::readNumber() {
	reset();
	AMF::Type type = followingType();
	if(type==AMF::Null) {
		reader.next(1);
		return 0;
	}
	if(type!=AMF::Number) {
		ERROR("Type %.2x is not a AMF Number type",type);
		return 0;
	}
	reader.next(1);
	double result;
	reader >> result;
	return result;
}

Int32 AMFReader::readInteger() {
	reset();
	AMF::Type type = followingType();
	if(type==AMF::Null) {
		reader.next(1);
		return 0;
	}
	if(type!=AMF::Integer && type!=AMF::Number) {
		ERROR("Type %.2x is not a AMF Integer type",type);
		return 0;
	}
	reader.next(1);
	if(type==AMF::Number) {
		double result;
		reader >> result;
		return (Int32)result;
	}
	// Forced in AMF3 here!
	UInt32 value = reader.read7BitValue();
	if(value>268435455)
		value-=(1<<29);
	return value;
}

bool AMFReader::readBoolean() {
	reset();
	AMF::Type type = followingType();
	if(type==AMF::Null) {
		reader.next(1);
		return false;
	}
	if(type!=AMF::Boolean) {
		ERROR("Type %.2x is not a AMF Boolean type",type);
		return false;
	}
	if(_amf3)
		return reader.read8()== AMF3_FALSE ? false : true;
	reader.next(1);
	return reader.read8()==0x00 ? false : true;
}

BinaryReader& AMFReader::readRawObjectContent() {
	reset();
	return reader;
}

BinaryReader& AMFReader::readByteArray(UInt32& size) {
	reset();

	AMF::Type type = followingType();
	if(type==AMF::Null) {
		reader.next(1);
		return BinaryReader::BinaryReaderNull;
	}
	if(type!=AMF::ByteArray) {
		ERROR("Type %.2x is not a AMF ByteArray type",type);
		return BinaryReader::BinaryReaderNull;
	}
	reader.next(1);

	// Forced in AMF3 here!
	UInt32 reference = reader.position();
	size = reader.read7BitValue();
	bool isInline = size&0x01;
	size >>= 1;
	if(isInline) {
		if(_referencing)
			_references.push_back(reference);
	} else {
		if(size>_references.size()) {
			ERROR("AMF3 reference not found")
			return BinaryReader::BinaryReaderNull;
		}
		_reset = reader.position();
		reader.reset(_references[size]);
		size = reader.read7BitValue()>>1;
	}
	return reader;
}

Timestamp AMFReader::readDate() {
	reset();

	AMF::Type type = followingType();
	if(type==AMF::Null) {
		reader.next(1);
		return Timestamp(0);
	}
	if(type!=AMF::Date) {
		ERROR("Type %.2x is not a AMF Date type",type);
		return Timestamp(0);
	}

	reader.next(1);
	double result=0;
	if(_amf3) {
		UInt32 flags = reader.read7BitValue();
		UInt32 reference = reader.position();
		bool isInline = flags&0x01;
		if(isInline) {
			if(_referencing)
				_references.push_back(reference);
			reader >> result;
		} else {
			flags >>= 1;
			if(flags>_references.size()) {
				ERROR("AMF3 reference not found")
				return Timestamp(0);
			}
			UInt32 reset = reader.position();
			reader.reset(_references[flags]);
			reader >> result;
			reader.reset(reset);
		}
		
		return Timestamp((Timestamp::TimeVal)result*1000);
	}
	reader >> result;
	reader.next(2); // Timezone, useless
	return Timestamp((Timestamp::TimeVal)result*1000);
}

void AMFReader::read(string& value) {
	reset();
	AMF::Type type = followingType();
	if(type==AMF::Null) {
		reader.next(1);
		value="";
		return;
	}
	if(type!=AMF::String) {
		ERROR("Type %.2x is not a AMF String type",type);
		return;
	}
	reader.next(1);
	if(_amf3) {
		readString(value);
		return;
	}
	if(current()==AMF_LONG_STRING) {
		reader.readRaw(reader.read32(),value);
		return;
	}
	reader.readString16(value);
}

bool AMFReader::readDictionary(bool& weakKeys) {
	reset();

	AMF::Type type = followingType();
	if(type==AMF::Null) {
		reader.next(1);
		return false;
	}
	if(type!=AMF::Dictionary) {
		ERROR("Type %.2x is not a AMF Dictionary type",type);
		return false;
	}

	// AMF3
	reader.next(1); // marker
	UInt32 reference = reader.position();
	UInt32 size = reader.read7BitValue();
	bool isInline = size&0x01;
	size >>= 1;

	if(!isInline && size>_references.size()) {
		ERROR("AMF3 reference not found")
		return false;
	}

	ObjectDef* pObjectDef = new ObjectDef(_amf3,AMF3_DICTIONARY);
	pObjectDef->dynamic=true;
	_objectDefs.push_back(pObjectDef);
	
	if(isInline) {
		if(_referencing)
			_references.push_back(reference);
		pObjectDef->count = size;
	} else {
		pObjectDef->reset = reader.position();
		reader.reset(_references[size]);
		pObjectDef->count = reader.read7BitValue()>>1;
	}
	pObjectDef->count *= 2;
	weakKeys = reader.read8()&0x01;

	return true;
}

AMF::Type AMFReader::readKey() {
	reset();
	if(_objectDefs.size()==0) {
		ERROR("AMFReader::readKey/AMFReader::readValue called without a AMFReader::readDictionary before");
		return AMF::End;
	}

	ObjectDef& objectDef = *_objectDefs.back();
	_amf3 = objectDef.amf3;

	if(objectDef.arrayType != AMF3_DICTIONARY) {
		ERROR("AMFReader::readKey/AMFReader::readValue must be called after a AMFReader::readDictionary");
		return AMF::End;
	}

	if(objectDef.count==0) {
		if(objectDef.reset>0)
			reader.reset(objectDef.reset);
		delete &objectDef;
		_objectDefs.pop_back();
		return AMF::End;
	}
	--objectDef.count;
	return followingType();
}

bool AMFReader::readArray() {
	reset();

	AMF::Type type = followingType();
	if(type==AMF::Null) {
		reader.next(1);
		return false;
	}
	if(type!=AMF::Array) {
		ERROR("Type %.2x is not a AMF Array type",type);
		return false;
	}

	if(!_amf3) {
		ObjectDef* pObjectDef = new ObjectDef(_amf3,current());
		_objectDefs.push_back(pObjectDef);
		pObjectDef->dynamic=true;
		if(_referencing)
			_amf0References.push_back(reader.position());
		if(_amf0Reset)
			pObjectDef->reset = _amf0Reset;
		reader.next(1);
		UInt32 count = reader.read32();
		if(pObjectDef->arrayType==AMF_STRICT_ARRAY)
			pObjectDef->count = count;
		return true;
	}
	
	// AMF3
	reader.next(1); // marker
	UInt32 reference = reader.position();
	UInt32 size = reader.read7BitValue();
	bool isInline = size&0x01;
	size >>= 1;

	if(!isInline && size>_references.size()) {
		ERROR("AMF3 reference not found")
		return false;
	}

	ObjectDef* pObjectDef = new ObjectDef(_amf3,AMF3_ARRAY);
	pObjectDef->dynamic=true;
	_objectDefs.push_back(pObjectDef);
	
	if(isInline) {
		if(_referencing)
			_references.push_back(reference);
		pObjectDef->count = size;
	} else {
		pObjectDef->reset = reader.position();
		reader.reset(_references[size]);
		pObjectDef->count = reader.read7BitValue()>>1;
	}

	return true;
}


bool AMFReader::readObject(string& type) {
	reset();

	AMF::Type marker = followingType();
	if(marker==AMF::Null) {
		reader.next(1);
		return false;
	}
	if(marker!=AMF::Object) {
		ERROR("Type %.2x is not a AMF Object type",marker);
		return false;
	}

	if(!_amf3) {
		if(_referencing)
			_amf0References.push_back(reader.position());
		if(current()==AMF_BEGIN_TYPED_OBJECT) {
			reader.next(1);
			readString(type);
		} else
			reader.next(1);
		ObjectDef* pObjectDef = new ObjectDef(_amf3);
		_objectDefs.push_back(pObjectDef);
		if(_amf0Reset)
			pObjectDef->reset = _amf0Reset;
		pObjectDef->dynamic = true;
		return true;
	}
	
	// AMF3
	reader.next(1); // marker
	UInt32 reference = reader.position();
	UInt32 flags = reader.read7BitValue();
	bool isInline = flags&0x01;
	flags >>= 1;

	if(!isInline && flags>_references.size()) {
		ERROR("AMF3 reference not found")
		return false;
	}

	ObjectDef* pObjectDef = new ObjectDef(_amf3);
	_objectDefs.push_back(pObjectDef);
	
	if(isInline) {
		if(_referencing)
			_references.push_back(reference);
	} else {
		pObjectDef->reset = reader.position();
		reader.reset(_references[flags]);
		flags = reader.read7BitValue()>>1;
	}

	// classdef reading
	isInline = flags&0x01; 
	flags >>= 1;
	UInt32 reset=0;
	if(isInline) {
		 _classDefReferences.push_back(reference);
		readString(type);
	} else if(flags<=_classDefReferences.size()) {
		reset = reader.position();
		reader.reset(_classDefReferences[flags]);
		flags = reader.read7BitValue()>>2;
		readString(type);
	} else {
		ERROR("AMF3 classDef reference not found")
		flags=0x02; // emulate dynamic class without hard properties
	}

	if(flags&0x01)
		pObjectDef->externalizable=true;
	else if(flags&0x02)
		pObjectDef->dynamic=true;
	flags>>=2;

	if(!pObjectDef->externalizable) {
		pObjectDef->hardProperties.resize(flags);
		list<string>::iterator it;
		for(it=pObjectDef->hardProperties.begin();it!=pObjectDef->hardProperties.end();++it)
			readString(*it);
	}
	
	if(reset>0)
		reader.reset(reset); // reset classdef

	return true;
}

AMF::Type AMFReader::readItem(string& name) {
	reset();
	if(_objectDefs.size()==0) {
		ERROR("AMFReader::readItem called without a AMFReader::readObject or a AMFReader::readArray before");
		return AMF::End;
	}

	ObjectDef& objectDef = *_objectDefs.back();
	_amf3 = objectDef.amf3;
	bool end=false;

	if(objectDef.arrayType == AMF3_DICTIONARY) {
		ERROR("AMFReader::readItem on a dictionary, used AMFReader::readKey and AMFReader::readValue rather");
		return AMF::End;
	}

	if(objectDef.hardProperties.size()>0) {
		name = objectDef.hardProperties.front();
		objectDef.hardProperties.pop_front();

	} else if(objectDef.arrayType == AMF_STRICT_ARRAY) {
		if(objectDef.count==0)
			end=true;
		else {
			--objectDef.count;
			name="";
		}

	} else if(!objectDef.dynamic) {
		if(objectDef.externalizable) {
			objectDef.externalizable=false;
			return AMF::RawObjectContent;
		}
		end=true;

	} else {
		readString(name);
		if(name.empty()) {
			if(objectDef.arrayType == AMF3_ARRAY) {
				objectDef.arrayType = AMF_STRICT_ARRAY;
				return readItem(name);
			}
			end=true;
		} else if(objectDef.arrayType) {
			int index;
			if(NumberParser::tryParse(name,index) && index>=0)
				name="";
		}
	}

	if(end) {
		if(!_amf3 && objectDef.arrayType!=AMF_STRICT_ARRAY) {
			UInt8 marker = reader.read8();
			if(marker!=AMF_END_OBJECT)
				ERROR("AMF0 end marker object absent");
		}
		if(objectDef.reset>0)
			reader.reset(objectDef.reset);
		delete &objectDef;
		_objectDefs.pop_back();
		return AMF::End;
	}
	
	return followingType();
}


void AMFReader::readString(string& value) {
	if(!_amf3) {
		reader.readString16(value);
		return;
	}
	UInt32 reference = reader.position();
	UInt32 size = reader.read7BitValue();
	bool isInline = size&0x01;
	size >>= 1;
	if(isInline) {
		reader.readRaw(size,value);
		if(!value.empty())
			_stringReferences.push_back(reference);
	} else {
		if(size>_stringReferences.size()) {
			ERROR("AMF3 string reference not found")
			return;
		}
		UInt32 reset = reader.position();
		reader.reset(_stringReferences[size]);
		reader.readRaw(reader.read7BitValue()>>1,value);
		reader.reset(reset);
	}
}


AMF::Type AMFReader::followingType() {
	reset();
	if(_amf3!=reader.position()) {
		if(_objectDefs.size()>0)
			_amf3=_objectDefs.back()->amf3;
		else
			_amf3=0;
	}
	if(!available())
		return AMF::End;
	
	UInt8 type = current();
	if(!_amf3 && type==AMF_AVMPLUS_OBJECT) {
		reader.next(1);
		_amf3=reader.position();
		if(!available())
			return AMF::End;
		type = current();
	}
	
	if(_amf3) {
		switch(type) {
			case AMF3_UNDEFINED:
			case AMF3_NULL:
				return AMF::Null;
			case AMF3_FALSE:
			case AMF3_TRUE:
				return AMF::Boolean;
			case AMF3_INTEGER:
				return AMF::Integer;
			case AMF3_NUMBER:
				return AMF::Number;
			case AMF3_STRING:
				return AMF::String;
			case AMF3_DATE:
				return AMF::Date;
			case AMF3_ARRAY:
				return AMF::Array;
			case AMF3_DICTIONARY:
				return AMF::Dictionary;
			case AMF3_OBJECT:
				return AMF::Object;
			case AMF3_BYTEARRAY:
				return AMF::ByteArray;
			default:
				ERROR("Unknown AMF3 type %.2x",type)
				reader.next(1);
				return followingType();
		}
	}
	switch(type) {
		case AMF_UNDEFINED:
		case AMF_NULL:
			return AMF::Null;
		case AMF_BOOLEAN:
			return AMF::Boolean;
		case AMF_NUMBER:
			return AMF::Number;
		case AMF_LONG_STRING:
		case AMF_STRING:
			return AMF::String;
		case AMF_MIXED_ARRAY:
		case AMF_STRICT_ARRAY:
			return AMF::Array;
		case AMF_DATE:
			return AMF::Date;
		case AMF_BEGIN_OBJECT:
		case AMF_BEGIN_TYPED_OBJECT:
			return AMF::Object;
		case AMF_REFERENCE: {
			reader.next(1);
			UInt16 reference = reader.read16();
			if(reference>_amf0References.size()) {
				ERROR("AMF0 reference not found")
				return followingType();
			}
			_amf0Reset = reader.position();
			reader.reset(_amf0References[reference]);
			return followingType();
		}
		case AMF_END_OBJECT:
			ERROR("AMF end object type without begin object type before")
			reader.next(1);
			return followingType();
		case AMF_UNSUPPORTED:
			WARN("Unsupported type in AMF format")
			reader.next(1);
			return followingType();
		default:
			ERROR("Unknown AMF type %.2x",type)
			reader.next(1);
			return followingType();
	}
}



void AMFReader::readSimpleObject(AMFSimpleObject& object) {
	string type;
	if(!readObject(type))
		return;

	if(!type.empty())
		WARN("Object seems not be a simple object because it has a %s type",type.c_str());

	string name;
	AMF::Type nextType;
	while((nextType = readItem(name))!=AMF::End) {
		switch(nextType) {
			case AMF::Null:
				readNull();
				object.setNull(name);
				break;
			case AMF::Boolean:
				object.setBoolean(name,readBoolean());
				break;
			case AMF::Integer:
				object.setInteger(name,readInteger());
				break;
			case AMF::String: {
				string value;
				read(value);
				object.setString(name,value);
				break;
			}
			case AMF::Number:
				object.setNumber(name,readNumber());
				break;
			case AMF::Date: {
				Timestamp date = readDate();
				object.setDate(name,date);
				break;
			}
			default:
				ERROR("AMF %u type unsupported in an AMFSimpleObject conversion",nextType);
				reader.next(1);
		}
	}
}



} // namespace Cumulus
