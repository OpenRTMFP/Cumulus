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

#include "MemoryStream.h"

using namespace std;

namespace Cumulus {

MemoryStreamBuf::MemoryStreamBuf(char* pBuffer, streamsize bufferSize): _pBuffer(pBuffer),_bufferSize(bufferSize),_written(0) {
	setg(_pBuffer, _pBuffer, _pBuffer + _bufferSize);
	setp(_pBuffer, _pBuffer + _bufferSize);
}

MemoryStreamBuf::MemoryStreamBuf(MemoryStreamBuf& other): _pBuffer(other._pBuffer),_bufferSize(other._bufferSize),_written(other._written) {
	setg(_pBuffer, other.gCurrent(), _pBuffer + _bufferSize);
	setp(_pBuffer, other.pCurrent(),_pBuffer + _bufferSize);
}


MemoryStreamBuf::~MemoryStreamBuf() {
}

void MemoryStreamBuf::position(streampos pos) {
	written(); // Save nb char written
	setg(_pBuffer, _pBuffer+pos, _pBuffer + _bufferSize);
	setp(_pBuffer, _pBuffer+pos, _pBuffer + _bufferSize);
}

void MemoryStreamBuf::resize(std::streamsize newSize) {
	_bufferSize = newSize;
	setp(_pBuffer, pCurrent(), _pBuffer + _bufferSize);
	setg(_pBuffer, gCurrent(), _pBuffer + _bufferSize);
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


MemoryIOS::MemoryIOS(char* pBuffer, std::streamsize bufferSize):_buf(pBuffer, bufferSize) {
	poco_ios_init(&_buf);
}
MemoryIOS::MemoryIOS(MemoryIOS& other):_buf(other._buf) {
	poco_ios_init(&_buf);
}

MemoryIOS::~MemoryIOS() {
}

void MemoryIOS::reset(streampos newPos,streamsize newSize) {
	if(newSize>0)
		rdbuf()->resize(newSize);
	if(newPos>=rdbuf()->size())
		newPos = rdbuf()->size()-1;
	rdbuf()->position(newPos);
	clear();
}



MemoryInputStream::MemoryInputStream(const char* pBuffer, std::streamsize bufferSize): 
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
