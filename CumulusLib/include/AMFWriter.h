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

#pragma once

#include "Cumulus.h"
#include "BinaryWriter.h"
#include "AMFSimpleObject.h"
#include <list>

namespace Cumulus {

class AMFWriter {
public:
	AMFWriter(BinaryWriter& writer);
	~AMFWriter();

	bool repeat(Poco::UInt32 reference);

	// slow
	void writeSimpleObject(const AMFSimpleObject& object);

	// fast
	void beginObject();
	void beginObject(const std::string& type);
	void writeObjectProperty(const std::string& name);
	void writeObjectProperty(const std::string& name,const Poco::Timestamp& date);
	void writeObjectProperty(const std::string& name,double value);
	void writeObjectProperty(const std::string& name,Poco::Int32 value);
	void writeObjectProperty(const std::string& name,const std::string& value);
	void writeObjectProperty(const std::string& name,const std::vector<Poco::UInt8>& data);
	void endObject();

	BinaryWriter&	beginExternalizableObject(const std::string& type);
	void			endExternalizableObject();


	void beginDictionary(Poco::UInt32 count,bool weakKeys=false);
	void endDictionary();

	void beginArray(Poco::UInt32 count);
	void beginObjectArray(Poco::UInt32 count);
	void endArray();

	void writeDate(const Poco::Timestamp& date);
	void writeInteger(Poco::Int32 value);
	void writeNumber(double value);
	void write(const std::string& value);
	void writeBoolean(bool value);
	void writeNull();
	BinaryWriter& writeByteArray(Poco::UInt32 size);
	void writeByteArray(const std::vector<Poco::UInt8>& data);
	void writeByteArray(const Poco::UInt8* data,Poco::UInt32 size);

	void writePropertyName(const std::string& value);
	
	BinaryWriter& writer;

	const Poco::UInt32	lastReference;
	bool				amf0Preference;
private:
	void beginObject(const std::string& type,bool externalizable);
	void writeString(const std::string& value);

	std::map<std::string,Poco::UInt32>	_stringReferences;
	std::vector<Poco::UInt8>			_references;
	bool								_amf3;
	std::list<Poco::UInt32>				_lastObjectReferences;
};

inline void AMFWriter::writeByteArray(const std::vector<Poco::UInt8>& data) {
	writeByteArray(data.size()>0 ? &data[0] : NULL,data.size());
}

inline void AMFWriter::beginObject() {
	beginObject("",false);
}

inline void AMFWriter::beginObject(const std::string& type) {
	beginObject(type,false);
}


} // namespace Cumulus
