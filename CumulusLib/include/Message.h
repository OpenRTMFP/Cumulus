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
	enum Type {
		EMPTY			=0x00,
		AUDIO			=0x08,
		VIDEO			=0x09,
		AMF_WITH_HANDLER=0x14,
		AMF				=0x0F
	};

	Message(std::istream& istr,bool repeatable);
	virtual ~Message();

	BinaryReader&			reader(Poco::UInt32& size);
	BinaryReader&			reader(Poco::UInt32 fragment,Poco::UInt32& size);
	virtual BinaryReader&	memAck(Poco::UInt32& available,Poco::UInt32& size);

	std::map<Poco::UInt32,Poco::UInt64>		fragments;
	const bool								repeatable;

private:
	virtual	Poco::UInt32	init(Poco::UInt32 position)=0;
	BinaryReader			_reader;
};

class MessageUnbuffered : public Message {
public:
	MessageUnbuffered(const Poco::UInt8* data,Poco::UInt32 size,const Poco::UInt8* memAckData=NULL,Poco::UInt32 memAckSize=0);
	virtual ~MessageUnbuffered();

private:
	BinaryReader&				memAck(Poco::UInt32& available,Poco::UInt32& size);

	Poco::UInt32				init(Poco::UInt32 position);

	MemoryInputStream			_stream;
	BinaryReader*				_pReaderAck;
	MemoryInputStream*			_pMemAck;
	Poco::Buffer<char>			_bufferAck;
	Poco::UInt32				_size;
};


class MessageBuffered : public Message {
public:
	MessageBuffered();
	virtual ~MessageBuffered();

	AMFWriter			amfWriter;
	BinaryWriter		rawWriter;
	
private:
	Poco::UInt32		init(Poco::UInt32 position);

	BinaryStream		_stream;
};

class MessageNull : public MessageBuffered {
public:
	MessageNull();
	virtual ~MessageNull();
};


} // namespace Cumulus
