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
#include "Session.h"
#include "Gateway.h"
#include <cstddef>

namespace Cumulus {

class Sessions
{
public:

	typedef std::map<Poco::UInt32,Session*>::const_iterator Iterator;

	Sessions(Gateway& gateway);
	virtual ~Sessions();

	Poco::UInt32	count() const;
	Poco::UInt32	nextId() const;
	Session* find(Poco::UInt32 id) const;
	Session* find(const Poco::UInt8* peerId) const;
	
	Session* add(Session* pSession);

	Iterator begin() const;
	Iterator end() const;
	
	bool	manage();
	void	clear();
protected:
	

private:
	Poco::UInt32					_nextId;
	std::map<Poco::UInt32,Session*>	_sessions;
	Gateway&						_gateway;
	Poco::UInt32					_oldCount;
};


inline Poco::UInt32	Sessions::nextId() const {
	return _nextId;
}

inline Sessions::Iterator Sessions::begin() const {
	return _sessions.begin();
}

inline Poco::UInt32 Sessions::count() const {
	return _sessions.size();
}

inline Sessions::Iterator Sessions::end() const {
	return _sessions.end();
}



} // namespace Cumulus
