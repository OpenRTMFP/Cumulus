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


class MemoryStreamBuf;
class ScopedMemoryClip {
public:
	ScopedMemoryClip(MemoryStreamBuf& buffer,Poco::UInt32 offset);
	~ScopedMemoryClip();
private:
	Poco::UInt32		_offset;
	MemoryStreamBuf&   _buffer;
};


class MemoryStreamBuf: public std::streambuf {
	friend class ScopedMemoryClip;
public:
	MemoryStreamBuf(char* pBuffer,Poco::UInt32 bufferSize);
	MemoryStreamBuf(MemoryStreamBuf&);
	~MemoryStreamBuf();

	
	void			next(Poco::UInt32 size);
	Poco::UInt32	written();
	void			written(Poco::UInt32 size);
	Poco::UInt32	size();
	void			resize(Poco::UInt32 newSize);
	char*			begin();
	void			position(Poco::UInt32 pos=0);
	char*			gCurrent();
	char*			pCurrent();
	
	void			clip(Poco::UInt32 offset);

private:
	void			clip(Poco::Int32 offset);

	virtual int overflow(int_type c);
	virtual int underflow();
	virtual int sync();

	Poco::UInt32	_written;
	char*			_pBuffer;
	Poco::UInt32	_bufferSize;

	MemoryStreamBuf();
	MemoryStreamBuf& operator = (const MemoryStreamBuf&);
};

inline void MemoryStreamBuf::clip(Poco::UInt32 offset) {
	clip((Poco::Int32)offset);
}

/// inlines

inline Poco::UInt32 MemoryStreamBuf::size() {
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
	MemoryIOS(char* pBuffer,Poco::UInt32 bufferSize);
		/// Creates the basic stream.
	MemoryIOS(MemoryIOS&);
	~MemoryIOS();
		/// Destroys the stream.

	MemoryStreamBuf* rdbuf();
		/// Returns a pointer to the underlying streambuf.

	virtual char*	current()=0;
	void			reset(Poco::UInt32 newPos);
	void			resize(Poco::UInt32 newSize);
	char*			begin();
	void			next(Poco::UInt32 size);
	Poco::UInt32	available();
	void			clip(Poco::UInt32 offset);
		
private:
	MemoryStreamBuf _buf;
};

/// inlines
inline char* MemoryIOS::begin() {
	return rdbuf()->begin();
}
inline void MemoryIOS::clip(Poco::UInt32 offset) {
	rdbuf()->clip(offset);
}
inline void MemoryIOS::resize(Poco::UInt32 newSize) {
	rdbuf()->resize(newSize);
}
inline void MemoryIOS::next(Poco::UInt32 size) {
	rdbuf()->next(size);
}
inline MemoryStreamBuf* MemoryIOS::rdbuf() {
	return &_buf;
}
//////////

class MemoryInputStream: public MemoryIOS, public std::istream
	/// An input stream for reading from a memory area.
{
public:
	MemoryInputStream(const char* pBuffer,Poco::UInt32 bufferSize);
		/// Creates a MemoryInputStream for the given memory area,
		/// ready for reading.
	MemoryInputStream(MemoryInputStream&);
	~MemoryInputStream();
		/// Destroys the MemoryInputStream.
	char*			current();
};



inline char* MemoryInputStream::current() {
	return rdbuf()->gCurrent();
}
//////////

class MemoryOutputStream: public MemoryIOS, public std::ostream
	/// An input stream for reading from a memory area.
{
public:
	MemoryOutputStream(char* pBuffer,Poco::UInt32 bufferSize);
		/// Creates a MemoryOutputStream for the given memory area,
		/// ready for writing.
	MemoryOutputStream(MemoryOutputStream&);
	~MemoryOutputStream();
		/// Destroys the MemoryInputStream.
	
	Poco::UInt32	written();
	void			written(Poco::UInt32 size);
	char*			current();
};

/// inlines
inline Poco::UInt32 MemoryOutputStream::written() {
	return rdbuf()->written();
}
inline void MemoryOutputStream::written(Poco::UInt32 size) {
	rdbuf()->written(size);
}
inline char* MemoryOutputStream::current() {
	return rdbuf()->pCurrent();
}
//////////


} // namespace Cumulus
