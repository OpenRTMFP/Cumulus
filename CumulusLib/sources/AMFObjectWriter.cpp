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

#include "AMFObjectWriter.h"

using namespace std;

namespace Cumulus {

AMFObjectWriter::AMFObjectWriter(AMFWriter& writer) : writer(writer),_end(true) {
	writer.beginObject();
}

AMFObjectWriter::AMFObjectWriter(const AMFObjectWriter& other) : writer(other.writer),_end(true) {
	((AMFObjectWriter&)other)._end = false;
}


AMFObjectWriter::~AMFObjectWriter() {
	if(_end)
		writer.endObject();
}


} // namespace Cumulus
