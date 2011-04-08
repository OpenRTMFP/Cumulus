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


class AMFWriter {
public:
	AMFWriter(BinaryWriter& writer);
	~AMFWriter();

	void writeResponseHeader(const std::string& key,double callbackHandle);

	// slow
	void writeObject(const AMFObject& amfObject);

	// fast
	void beginObject();
	void writeObjectProperty(const std::string& name,double value);
	void writeObjectProperty(const std::string& name,const std::string& value);
	void writeObjectProperty(const std::string& name,const std::vector<Poco::UInt8>& data);
	void endObject();

	void writeNumber(double value);
	void write(const std::string& value);
	void writeBool(bool value);
	void writeNull();
	void writeByteArray(const std::vector<Poco::UInt8>& data);
	
private:
	BinaryWriter& _writer;
};

inline void AMFWriter::beginObject() {
	_writer.write8(AMF_BEGIN_OBJECT); // mark deb
}

inline void AMFWriter::writeNull() {
	_writer.write8(AMF_NULL); // marker
}


} // namespace Cumulus
