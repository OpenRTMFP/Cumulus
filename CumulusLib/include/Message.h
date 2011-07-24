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
#include "AMFWriter.h"
#include "BinaryStream.h"
#include "BinaryReader.h"
#include "MemoryStream.h"
#include "Poco/Buffer.h"
#include <list>

namespace Cumulus {


class Message {
public:
	Message(std::istream& istr,bool repeatable);
	virtual ~Message();

	BinaryReader&			reader(Poco::UInt32& size);
	virtual BinaryReader&	memAck(Poco::UInt32& size);
	virtual	Poco::UInt32	init(Poco::UInt32 position)=0;
	
	std::list<Poco::UInt32>		fragments;
	Poco::UInt32				startStage;
	const bool					repeatable;

private:
	BinaryReader			_reader;
};

inline BinaryReader& Message::memAck(Poco::UInt32& size) {
	return reader(size);
}

class MessageUnbuffered : public Message {
public:
	MessageUnbuffered(const Poco::UInt8* data,Poco::UInt32 size,const Poco::UInt8* memAckData=NULL,Poco::UInt32 memAckSize=0);
	virtual ~MessageUnbuffered();

	Poco::UInt32		init(Poco::UInt32 position);
	BinaryReader&		memAck(Poco::UInt32& size);
	
private:
	MemoryInputStream			_stream;
	BinaryReader*				_pReaderAck;
	MemoryInputStream*			_pMemAck;
	Poco::Buffer<char>			_bufferAck;
};

class MessageBuffered : public Message {
public:
	MessageBuffered();
	virtual ~MessageBuffered();

	Poco::UInt32		init(Poco::UInt32 position);

	AMFWriter			amfWriter;
	BinaryWriter		rawWriter;
	
private:
	BinaryStream		_stream;
};


} // namespace Cumulus
