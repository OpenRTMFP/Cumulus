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
#include "PacketReader.h"
#include "AMFSimpleObject.h"
#include <list>

namespace Cumulus {


class ObjectDef;
class AMFReader {
public:
	AMFReader(PacketReader& reader);
	~AMFReader();

	void			readSimpleObject(AMFSimpleObject& object);

	void			read(std::string& value);
	double			readNumber();
	Poco::Int32		readInteger();
	bool			readBoolean();
	BinaryReader&	readByteArray(Poco::UInt32& size);
	Poco::Timestamp	readDate();

	bool			readObject(std::string& type);
	bool			readArray();
	bool			readDictionary(bool& weakKeys);
	AMF::Type		readKey();
	AMF::Type		readValue();
	AMF::Type		readItem(std::string& name);
	BinaryReader&	readRawObjectContent();

	void			readNull();
	AMF::Type		followingType();

	bool			available();

	void			startReferencing();
	void			stopReferencing();
	
	PacketReader&	reader;
private:
	void							readString(std::string& value);
	Poco::UInt8						current();
	void							reset();
	std::list<ObjectDef*>			_objectDefs;
	std::vector<Poco::UInt32>		_stringReferences;
	std::vector<Poco::UInt32>		_classDefReferences;
	std::vector<Poco::UInt32>		_references;
	std::vector<Poco::UInt32>		_amf0References;
	Poco::UInt32					_amf0Reset;
	Poco::UInt32					_reset;
	Poco::UInt32					_amf3;
	bool							_referencing;
};

inline AMF::Type AMFReader::readValue() {
	return readKey();
}

inline Poco::UInt8 AMFReader::current() {
	return *reader.current();
}

inline void AMFReader::startReferencing() {
	_referencing=true;
}

inline void	AMFReader::stopReferencing() {
	_referencing=false;
}



} // namespace Cumulus
