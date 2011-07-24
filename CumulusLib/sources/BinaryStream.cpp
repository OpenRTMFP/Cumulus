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


BinaryBuffer::BinaryBuffer() {
}
BinaryBuffer::~BinaryBuffer() {
}


BinaryBuffer::int_type BinaryBuffer::readFromDevice() {
    if(size()==0)
		return EOF;
    return _buf.sbumpc();
}

inline Poco::UInt32 BinaryBuffer::size() {
	int result = _buf.pubseekoff(0,std::ios_base::cur,std::ios_base::out) - _buf.pubseekoff(0,std::ios_base::cur,std::ios_base::in);
	if(result<0)
		result=0;
	return (UInt32)result;
}

BinaryIOS::BinaryIOS() {
	poco_ios_init(&_buf);
}
BinaryIOS::~BinaryIOS() {
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

void BinaryStream::resetReading(UInt32 position) {
	rdbuf()->pubseekoff(position,ios::beg,ios_base::in);
    iostream::clear();
}



} // namespace Cumulus
