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

namespace Cumulus {


class Sessions
{
public:

	typedef std::map<Poco::UInt32,Session*>::const_iterator Iterator;

	Sessions();
	virtual ~Sessions();

	Session* find(Poco::UInt32 id);
	Session* find(const Poco::UInt8* peerId);
	
	Session* add(Session* pSession);

	Iterator begin() const;
	Iterator end() const;
	
	void	clear();
	void	manage();

	void	freqManage(Poco::UInt8 freq);
protected:
	

private:
	std::map<Poco::UInt32,Session*>	_sessions;
	Poco::Timestamp					_timeLastManage;
	int								_freqManage;
};

inline Sessions::Iterator Sessions::begin() const {
	return _sessions.begin();
}

inline void Sessions::freqManage(Poco::UInt8 freq) {
	_freqManage = freq*1000000;
}

inline Sessions::Iterator Sessions::end() const {
	return _sessions.end();
}



} // namespace Cumulus
