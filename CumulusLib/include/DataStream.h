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
#include "Poco/RWLock.h"
#include "Poco/Data/Session.h"

namespace Cumulus {


class DataStream : public Poco::Data::Session {
public:
	DataStream(const Poco::Data::Session& session,Poco::RWLock& rwLock);
	DataStream(DataStream& other);
	virtual ~DataStream();
private:
	bool			_unlock;
	Poco::RWLock&  _rwLock;
};

inline DataStream::DataStream(const Poco::Data::Session& session,Poco::RWLock& rwLock):_rwLock(rwLock),Poco::Data::Session(session),_unlock(true) {
}

inline DataStream::DataStream(DataStream& other):_rwLock(other._rwLock),Poco::Data::Session(other),_unlock(true) {
	other._unlock = false;
}

inline DataStream::~DataStream() {
	if(_unlock)
		_rwLock.unlock();
}


} // namespace Cumulus
