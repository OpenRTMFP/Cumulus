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
#include "PoolThread.h"
#include <vector>

namespace Cumulus {

class PoolThreads {
public:
	PoolThreads(Poco::UInt32 threadsAvailable=0);
	virtual ~PoolThreads();

	void			clear();
	Poco::UInt32	threadsAvailable();

	PoolThread*	enqueue(Poco::AutoPtr<WorkThread> pWork,PoolThread* pThread=NULL);

private:
	std::vector<PoolThread*>	_threads;
	Poco::FastMutex				_mutex;
};

inline Poco::UInt32	PoolThreads::threadsAvailable() {
	return _threads.size();
}


} // namespace Cumulus
