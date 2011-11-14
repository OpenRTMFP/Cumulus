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
#include "AMFObject.h"

namespace Cumulus {


class CUMULUS_API AMFWriter {
public:
	AMFWriter(BinaryWriter& writer);
	~AMFWriter();

	void writeResponseHeader(const std::string& key,double callbackHandle);

	// slow
	void writeObject(const AMFObject& amfObject);

	// fast
	void beginObject();
	void beginSubObject(const std::string& name);
	void writeObjectProperty(const std::string& name,double value);
	void writeObjectProperty(const std::string& name,const std::string& value);
	void writeObjectArrayProperty(const std::string& name,Poco::UInt32 count);
	void writeObjectProperty(const std::string& name,const char* value,Poco::UInt16 size);
	void writeObjectProperty(const std::string& name,const std::vector<Poco::UInt8>& data);
	void endObject();

	void writeArray(Poco::UInt32 count);

	void writeNumber(double value);
	void write(const std::string& value);
	void write(const char* value,Poco::UInt16 size);
	void writeBool(bool value);
	void writeNull();
	BinaryWriter& writeByteArray(Poco::UInt32 size);
	void writeByteArray(const std::vector<Poco::UInt8>& data);
	void writeByteArray(const Poco::UInt8* data,Poco::UInt32 size);
	
	BinaryWriter& writer;
};


inline void AMFWriter::beginObject() {
	writer.write8(AMF_BEGIN_OBJECT); // mark deb
}

inline void AMFWriter::writeNull() {
	writer.write8(AMF_NULL); // marker
}

inline void AMFWriter::writeByteArray(const std::vector<Poco::UInt8>& data) {
	writeByteArray(data.size()>0 ? &data[0] : NULL,data.size());
}


} // namespace Cumulus
