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
#include "PacketWriter.h"
#include <list>

namespace Cumulus {


class Message {
public:
	Message();
	virtual ~Message();

	AMFWriter					amfWriter;
	BinaryWriter				rawWriter;

	std::streamsize				available();
	void						reset();
	void						read(PacketWriter& writer,std::streamsize size);
	std::list<Poco::UInt32>		fragments;
	Poco::UInt32				startStage;

private:
	BinaryStream				_stream;
};

inline std::streamsize Message::available() {
	return _stream.size();
}



} // namespace Cumulus
