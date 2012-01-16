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
#include "Poco/BinaryReader.h"
#include "Poco/Net/SocketAddress.h"

namespace Cumulus {


class BinaryReader : public Poco::BinaryReader {
public:
	BinaryReader(std::istream& istr);
	virtual ~BinaryReader();

	Poco::UInt32	read7BitValue();
	Poco::UInt32	read7BitEncoded();
	void			readString(std::string& value);
	void			readRaw(Poco::UInt8* value,Poco::UInt32 size);
	void			readRaw(char* value,Poco::UInt32 size);
	void			readRaw(Poco::UInt32 size,std::string& value);
	void			readString8(std::string& value);
	void			readString16(std::string& value);
	Poco::UInt8		read8();
	Poco::UInt16	read16();
	Poco::UInt32	read32();
	bool			readAddress(Address& address);

	static BinaryReader BinaryReaderNull;
};

inline void BinaryReader::readRaw(Poco::UInt8* value,Poco::UInt32 size) {
	Poco::BinaryReader::readRaw((char*)value,size);
}
inline void BinaryReader::readRaw(char* value,Poco::UInt32 size) {
	Poco::BinaryReader::readRaw(value,size);
}
inline void BinaryReader::readRaw(Poco::UInt32 size,std::string& value) {
	Poco::BinaryReader::readRaw(size,value);
}

inline void BinaryReader::readString8(std::string& value) {
	readRaw(read8(),value);
}
inline void BinaryReader::readString16(std::string& value) {
	readRaw(read16(),value);
}

} // namespace Cumulus
