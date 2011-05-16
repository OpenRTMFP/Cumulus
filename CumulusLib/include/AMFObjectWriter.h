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
#include "AMFWriter.h"

namespace Cumulus {


class AMFObjectWriter {
public:
	AMFObjectWriter(AMFWriter& writer);
	AMFObjectWriter(const AMFObjectWriter& other);
	~AMFObjectWriter();

	// slow
	void writeObject(const AMFObject& amfObject);

	// fast
	void write(const std::string& name,double value);
	void write(const std::string& name,const std::string& value);
	void write(const std::string& name,const char* value,Poco::UInt16 size);
	void write(const std::string& name,const std::vector<Poco::UInt8>& data);
	
private:
	bool		_end;
	AMFWriter&	_writer;
};

inline void AMFObjectWriter::writeObject(const AMFObject& amfObject) {
	_writer.writeObject(amfObject);
}

inline void AMFObjectWriter::write(const std::string& name,double value) {
	_writer.writeObjectProperty(name,value);
}
inline void AMFObjectWriter::write(const std::string& name,const std::string& value) {
	_writer.writeObjectProperty(name,value);
}
inline void AMFObjectWriter::write(const std::string& name,const char* value,Poco::UInt16 size) {
	_writer.writeObjectProperty(name,value,size);
}
inline void AMFObjectWriter::write(const std::string& name,const std::vector<Poco::UInt8>& data) {
	_writer.writeObjectProperty(name,data);
}

} // namespace Cumulus
