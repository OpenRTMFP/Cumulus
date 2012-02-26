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
#include "Address.h"
#include "Poco/BinaryWriter.h"
#include "Poco/Net/SocketAddress.h"

namespace Cumulus {


class BinaryWriter : public Poco::BinaryWriter {
public:
	BinaryWriter(std::ostream& ostr);
	virtual ~BinaryWriter();

	void writeRaw(const Poco::UInt8* value,Poco::UInt32 size);
	void writeRaw(const char* value,Poco::UInt32 size);
	void writeRaw(const std::string& value);
	void write8(Poco::UInt8 value);
	void write16(Poco::UInt16 value);
	void write32(Poco::UInt32 value);
	void writeString8(const std::string& value);
	void writeString8(const char* value,Poco::UInt8 size);
	void writeString16(const std::string& value);
	void writeString16(const char* value,Poco::UInt16 size);
	void write7BitValue(Poco::UInt32 value);
	void write7BitLongValue(Poco::UInt64 value);
	void writeAddress(const Address& address,bool publicFlag);
	void writeAddress(const Poco::Net::SocketAddress& address,bool publicFlag);

	static BinaryWriter BinaryWriterNull;
};

inline void BinaryWriter::writeRaw(const Poco::UInt8* value,Poco::UInt32 size) {
	Poco::BinaryWriter::writeRaw((char*)value,size);
}
inline void BinaryWriter::writeRaw(const char* value,Poco::UInt32 size) {
	Poco::BinaryWriter::writeRaw(value,size);
}
inline void BinaryWriter::writeRaw(const std::string& value) {
	Poco::BinaryWriter::writeRaw(value);
}

inline void BinaryWriter::write8(Poco::UInt8 value) {
	(*this) << value;
}

inline void BinaryWriter::write16(Poco::UInt16 value) {
	(*this) << value;
}

inline void BinaryWriter::write32(Poco::UInt32 value) {
	(*this) << value;
}

} // namespace Cumulus
