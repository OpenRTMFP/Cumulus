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
#include "Poco/StreamUtil.h"
#include <streambuf>
#include <iostream>

namespace Cumulus {


class MemoryStreamBuf: public std::streambuf {
public:
	MemoryStreamBuf(char* pBuffer, std::streamsize bufferSize);
	MemoryStreamBuf(MemoryStreamBuf&);
	~MemoryStreamBuf();

	virtual int overflow(int_type c);

	virtual int underflow();

	virtual int sync();
	
	void			skip(std::streamsize size);
	std::streamsize written(std::streamsize size=-1);
	std::streamsize size();
	void			resize(std::streamsize newSize);
	char*			begin();
	void			position(std::streampos pos=0);
	char*			gCurrent();
	char*			pCurrent();

private:
	std::streamsize _written;
	char*			_pBuffer;
	std::streamsize _bufferSize;

	MemoryStreamBuf();
	MemoryStreamBuf& operator = (const MemoryStreamBuf&);
};

/// inlines

inline void MemoryStreamBuf::skip(std::streamsize size) {
	pbump(size);
	gbump(size);
}

inline std::streamsize MemoryStreamBuf::size() {
	return _bufferSize;
}
inline char* MemoryStreamBuf::begin() {
	return _pBuffer;
}
inline char* MemoryStreamBuf::gCurrent() {
	return gptr();
}
inline char* MemoryStreamBuf::pCurrent() {
	return pptr();
}
//////////


class MemoryIOS: public virtual std::ios
	/// The base class for MemoryInputStream and MemoryOutputStream.
	///
	/// This class is needed to ensure the correct initialization
	/// order of the stream buffer and base classes.
{
public:
	MemoryIOS(char* pBuffer, std::streamsize bufferSize);
		/// Creates the basic stream.
	MemoryIOS(MemoryIOS&);
	~MemoryIOS();
		/// Destroys the stream.

	MemoryStreamBuf* rdbuf();
		/// Returns a pointer to the underlying streambuf.

	void			reset(std::streampos newPos=0,std::streamsize newSize=0);
	char*			begin();
	void			skip(std::streamsize size);
		
private:
	MemoryStreamBuf _buf;
};

/// inlines
inline char* MemoryIOS::begin() {
	return rdbuf()->begin();
}
inline void MemoryIOS::skip(std::streamsize size) {
	rdbuf()->skip(size);
}
inline MemoryStreamBuf* MemoryIOS::rdbuf() {
	return &_buf;
}
//////////

class MemoryInputStream: public MemoryIOS, public std::istream
	/// An input stream for reading from a memory area.
{
public:
	MemoryInputStream(const char* pBuffer, std::streamsize bufferSize);
		/// Creates a MemoryInputStream for the given memory area,
		/// ready for reading.
	MemoryInputStream(MemoryInputStream&);
	~MemoryInputStream();
		/// Destroys the MemoryInputStream.

	std::streamsize available();
	char*			current();
};

/// inlines
inline std::streamsize MemoryInputStream::available() {
	return rdbuf()->size() - static_cast<std::streamsize>(rdbuf()->gCurrent()-rdbuf()->begin());
}

inline char* MemoryInputStream::current() {
	return rdbuf()->gCurrent();
}
//////////

class MemoryOutputStream: public MemoryIOS, public std::ostream
	/// An input stream for reading from a memory area.
{
public:
	MemoryOutputStream(char* pBuffer, std::streamsize bufferSize);
		/// Creates a MemoryOutputStream for the given memory area,
		/// ready for writing.
	MemoryOutputStream(MemoryOutputStream&);
	~MemoryOutputStream();
		/// Destroys the MemoryInputStream.
	
	std::streamsize written(std::streamsize size=-1);
	char*			current();
};

/// inlines
inline std::streamsize MemoryOutputStream::written(std::streamsize size) {
	return rdbuf()->written(size);
}
inline char* MemoryOutputStream::current() {
	return rdbuf()->pCurrent();
}
//////////


} // namespace Cumulus
