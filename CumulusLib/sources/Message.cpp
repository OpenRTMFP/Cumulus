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

#include "Message.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Message::Message() : rawWriter(_stream),amfWriter(rawWriter),startStage(0) {
	
}


Message::~Message() {

}

void Message::reset() {
	list<UInt32>::const_iterator it = fragments.begin();
	if(it==fragments.end()) {
		_stream.resetReading(0);
		return;
	}
	UInt32 fragment = *it;
	_stream.resetReading(fragment);
}

void Message::read(PacketWriter& writer,streamsize size) {
	_stream.read((char*)(writer.begin()+writer.position()),size);
	writer.next(size);
}


} // namespace Cumulus
