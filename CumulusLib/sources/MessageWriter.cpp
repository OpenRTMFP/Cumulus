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

#include "MessageWriter.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

MessageWriter::MessageWriter() : BinaryWriter(_stream),amf(*this),_startStage(0) {
	
}


MessageWriter::~MessageWriter() {

}

void MessageWriter::read(PacketWriter& writer,int size) {
	_stream.read((char*)(writer.begin()+writer.position()),size);
	writer.next(size);
}


} // namespace Cumulus
