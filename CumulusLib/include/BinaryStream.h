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
#include "Poco/UnbufferedStreamBuf.h"
#include <sstream>

namespace Cumulus {

class BinaryBuffer : public Poco::UnbufferedStreamBuf {
       
public:
	BinaryBuffer();
	~BinaryBuffer();
	Poco::UInt32		size();

private:
	std::streampos seekpos(std::streampos sp,std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);
	std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);
	std::streamsize xsputn(const char_type* s, std::streamsize n);
	std::streamsize xsgetn(char_type* p, std::streamsize count);
#if defined(_WIN32)
	std::streamsize _Xsgetn_s(char_type * _Ptr,size_t _Ptr_size, std::streamsize _Count);
#endif

	int_type readFromDevice();
	int_type writeToDevice(char_type);

	std::stringbuf  _buf;
};

inline BinaryBuffer::int_type BinaryBuffer::writeToDevice(char_type ch) {
	return _buf.sputc(ch);
}

inline std::streamsize BinaryBuffer::xsputn (const char_type* s, std::streamsize n){
        return _buf.sputn(s,n);
}

inline std::streampos BinaryBuffer::seekpos(std::streampos sp,std::ios_base::openmode which) {
       return _buf.pubseekpos(sp,which);
}

inline std::streampos BinaryBuffer::seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) {
        return _buf.pubseekoff(off,way,which);
}

inline std::streamsize BinaryBuffer::xsgetn(char_type* p,std::streamsize count) {
        return _buf.sgetn(p,count);
}

#if defined(_WIN32) && _MSC_VER<1600
inline std::streamsize BinaryBuffer::_Xsgetn_s(char_type * _Ptr,size_t _Ptr_size, std::streamsize _Count) {
        return _buf._Sgetn_s(_Ptr,_Ptr_size,_Count);
}
#endif


class BinaryIOS: public virtual std::ios {
       
public:
        BinaryBuffer* rdbuf();

protected:
        BinaryIOS();
        ~BinaryIOS();

private:
        BinaryBuffer  _buf;
};

inline BinaryBuffer* BinaryIOS::rdbuf() {
        return &_buf;
}



class BinaryStream : public BinaryIOS, public std::iostream {
public:
      BinaryStream();
      ~BinaryStream();

	  Poco::UInt32	  size();
      void            clear();
	  void            resetReading(Poco::UInt32 position);
      bool            empty();
private:
};

inline Poco::UInt32 BinaryStream::size() {
      return rdbuf()->size();
}


inline bool BinaryStream::empty() {
        return size()==0;
}


} // namespace Cumulus
