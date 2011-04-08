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

#include "BinaryStream.h"
#include "Poco/StreamCopier.h"
#include "Poco/NullStream.h"

using namespace std;
using namespace Poco;

namespace Cumulus {


BinaryBuffer::int_type BinaryBuffer::readFromDevice() {
        if(size()==0)
                return EOF;
        return _buf.sbumpc();
}

void BinaryBuffer::copyTo(char_type* data,streamsize size) {
    std::streampos precGPos = _buf.pubseekoff(0,ios_base::cur,ios_base::in);
    _buf.sgetn(data,size);
    _buf.pubseekpos(precGPos,ios_base::in);
}


BinaryStream::BinaryStream() : iostream(rdbuf()) {
}

BinaryStream::~BinaryStream() {
       
}

void BinaryStream::clear() {
    // vider le stream buf
    NullOutputStream nos;
    StreamCopier::copyStream(*this,nos);
    rdbuf()->pubseekoff(0,ios::beg);
	iostream::clear();
}

void BinaryStream::resetReading(std::streampos position) {
	rdbuf()->pubseekoff(position,ios::beg,ios_base::in);
    iostream::clear();
}



} // namespace Cumulus
