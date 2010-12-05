/* 
	Copyright 2010 cumulus.dev@gmail.com
 
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

#include "AMFWriter.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

AMFWriter::AMFWriter(PacketWriter& writer) : _writer(writer) {

}


AMFWriter::~AMFWriter() {

}

void AMFWriter::write(double value){
	_writer.write8(0x00); // marker
	_writer << value;
}

void AMFWriter::write(const string& value) {
	_writer.write8(0x02); // marker
	_writer.writeString16(value);
}

void AMFWriter::writeObjectProperty(const string& name,double value) {
	_writer.writeString16(name);
	write(value);
}

void AMFWriter::writeObjectProperty(const string& name,const string& value) {
	_writer.writeString16(name);
	write(value);
}

void AMFWriter::endObject() {
	// mark end
	_writer.write16(0); 
	_writer.write8(0x09);
}



} // namespace Cumulus
