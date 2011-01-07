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

#include "MemoryStream.h"

using namespace std;

namespace Cumulus {

MemoryStreamBuf::MemoryStreamBuf(char* pBuffer, streamsize bufferSize): _pBuffer(pBuffer),_bufferSize(bufferSize),_written(0) {
	setg(_pBuffer, _pBuffer,_pBuffer + _bufferSize);
	setp(_pBuffer, _pBuffer + _bufferSize);
}

MemoryStreamBuf::MemoryStreamBuf(MemoryStreamBuf& other): _pBuffer(other._pBuffer),_bufferSize(other._bufferSize),_written(other._written) {
	setg(_pBuffer,other.gCurrent(),_pBuffer + _bufferSize);
	setp(_pBuffer,_pBuffer + _bufferSize);
	pbump((int)(other.pCurrent()-_pBuffer));
}


MemoryStreamBuf::~MemoryStreamBuf() {
}

void MemoryStreamBuf::position(streampos pos) {
	written(); // Save nb char written
	setp(_pBuffer,_pBuffer + _bufferSize);
	if(pos<0)
		pos = 0;
	else if(pos>=_bufferSize)
		pos = _bufferSize-1;
	pbump((int)pos);
	setg(_pBuffer,_pBuffer+pos,_pBuffer + _bufferSize);
}

void MemoryStreamBuf::resize(streamsize newSize) {
	if(newSize<0)
		return;
	_bufferSize = newSize;
	int pos = gCurrent()-_pBuffer;
	if(pos>=_bufferSize)
		pos = _bufferSize-1;
	setg(_pBuffer,_pBuffer+pos,_pBuffer + _bufferSize);
	pos = pCurrent()-_pBuffer;
	if(pos>=_bufferSize)
		pos = _bufferSize-1;
	setp(_pBuffer,_pBuffer + _bufferSize);
	pbump(pos);
	if(_written>=_bufferSize)
		_written = _bufferSize-1;
}

void MemoryStreamBuf::clip(streampos offset) {
	if(offset>=_bufferSize)
		offset = _bufferSize-1;

	int gpos = gCurrent()-_pBuffer;
	int ppos = pCurrent()-_pBuffer;

	_pBuffer += offset;
	_bufferSize -= offset;
	
	if(gpos>=_bufferSize)
		gpos = _bufferSize-1;
	if(ppos>=_bufferSize)
		ppos = _bufferSize-1;

	setg(_pBuffer,_pBuffer+gpos,_pBuffer + _bufferSize);

	setp(_pBuffer,_pBuffer + _bufferSize);
	pbump(ppos);

	if(_written<offset)
		_written=0;
	else
		_written-=offset;
}

streamsize MemoryStreamBuf::written(streamsize size) {
	if(size>=0) {
		_written = size;
		return _written;
	}
	streamsize written = static_cast<streamsize>(pCurrent()-begin());
	if(written>_written) 
		_written = written;
	return _written;
}

int MemoryStreamBuf::overflow(int_type c) {
	return EOF;
}

int MemoryStreamBuf::underflow() {
	return EOF;
}

int MemoryStreamBuf::sync() {
	return 0;
}


MemoryIOS::MemoryIOS(char* pBuffer, streamsize bufferSize):_buf(pBuffer, bufferSize) {
	poco_ios_init(&_buf);
}
MemoryIOS::MemoryIOS(MemoryIOS& other):_buf(other._buf) {
	poco_ios_init(&_buf);
}

MemoryIOS::~MemoryIOS() {
}

void MemoryIOS::reset(streampos newPos) {
	rdbuf()->position(newPos);
	clear();
}



MemoryInputStream::MemoryInputStream(const char* pBuffer, streamsize bufferSize): 
	MemoryIOS(const_cast<char*>(pBuffer), bufferSize), istream(rdbuf()) {
}

MemoryInputStream::MemoryInputStream(MemoryInputStream& other):
	MemoryIOS(other), istream(rdbuf()) {
}

MemoryInputStream::~MemoryInputStream() {
}


MemoryOutputStream::MemoryOutputStream(char* pBuffer, streamsize bufferSize): 
	MemoryIOS(pBuffer, bufferSize), ostream(rdbuf()) {
}
MemoryOutputStream::MemoryOutputStream(MemoryOutputStream& other):
	MemoryIOS(other), ostream(rdbuf()) {
}

MemoryOutputStream::~MemoryOutputStream(){
}


} // namespace Cumulus
