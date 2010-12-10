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
#include "MemoryStream.h"
#include "Poco/BinaryWriter.h"
#include "Poco/Net/SocketAddress.h"

namespace Cumulus {


class PacketWriter: public Poco::BinaryWriter {
public:
	PacketWriter(int pos=0);
	PacketWriter(PacketWriter&);
	virtual ~PacketWriter();

	void writeRaw(const Poco::UInt8* value,int size);
	void writeRaw(const char* value,int size);
	void writeRaw(const std::string& value);
	void write32(Poco::UInt32 value);
	void write16(Poco::UInt16 value);
	void write8(Poco::UInt8 value);
	void writeString8(const std::string& value);
	void writeString16(const std::string& value);
	void writeString8(const char* value,Poco::UInt8 size);
	void writeString16(const char* value,Poco::UInt16 size);
	void writeRandom(Poco::UInt16 size);
	void writeAddress(const Poco::Net::SocketAddress& address);

	Poco::UInt8*	begin();
	int				size();
	int				position();
	
	void	clear(int pos=0);
	void	reset(int newPos=0);
	void	skip(int size);
	void	flush();
	
private:
	Poco::UInt8			_buff[MAX_SIZE_MSG];
	MemoryOutputStream	_memory;
	PacketWriter*		_pOther;
};

inline void PacketWriter::writeRaw(const Poco::UInt8* value,int size) {
	Poco::BinaryWriter::writeRaw((char*)value,size);
}
inline void PacketWriter::writeRaw(const char* value,int size) {
	Poco::BinaryWriter::writeRaw(value,size);
}
inline void PacketWriter::writeRaw(const std::string& value) {
	Poco::BinaryWriter::writeRaw(value);
}

inline void PacketWriter::write8(Poco::UInt8 value) {
	(*this) << value;
}

inline void PacketWriter::write16(Poco::UInt16 value) {
	(*this) << value;
}

inline void PacketWriter::write32(Poco::UInt32 value) {
	(*this) << value;
}

inline int PacketWriter::size() {
	return _memory.written();
}
inline int PacketWriter::position() {
	return _memory.current()-(char*)begin();
}
inline void PacketWriter::reset(int newPos) {
	_memory.reset(newPos);
}
inline void PacketWriter::skip(int size) {
	return _memory.skip(size);
}

inline Poco::UInt8* PacketWriter::begin() {
	return (Poco::UInt8*)_memory.begin();
}


} // namespace Cumulus
