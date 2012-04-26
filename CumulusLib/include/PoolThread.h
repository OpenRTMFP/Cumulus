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
#include "Startable.h"
#include "Poco/AtomicCounter.h"
#include "Poco/AutoPtr.h"
#include "Poco/NumberFormatter.h"
#include <list>

namespace Cumulus {

template <class RunnableType>
class PoolThread : private Startable {
public:
	PoolThread() : Startable("PoolThread"+Poco::NumberFormatter::format(++_Id))  {
	}
	~PoolThread() {
		clear();
	}

	void clear() {
		stop();
	}
	void push(Poco::AutoPtr<RunnableType>& pRunnable) {
		++_queue;
		Poco::ScopedLock<Poco::FastMutex> lock(_mutex);
		_runnables.push_back(pRunnable);
		start();
		wakeUp();
	}

	int queue() const {
		return _queue.value();
	}

private:
	void run() {

		for(;;) {

			WakeUpType wakeUpType = sleep(40000); // 40 sec of timeout
			
			for(;;) {
				RunnableType* pRunnable;
				{
					Poco::ScopedLock<Poco::FastMutex> lock(_mutex);
					if(_runnables.empty()) {
						if(wakeUpType!=WAKEUP) { // STOP or TIMEOUT
							if(wakeUpType==TIMEOUT)
								stop();
							return;
						}
						break;
					}
					pRunnable = _runnables.front();
				}

				pRunnable->run();
				
				{
					Poco::ScopedLock<Poco::FastMutex> lock(_mutex);
					_runnables.pop_front();
				}
				--_queue;
			}
		}
	}

	mutable Poco::FastMutex					_mutex;
	std::list<Poco::AutoPtr<RunnableType> >	_runnables;
	Poco::AtomicCounter						_queue;
	
	static Poco::UInt32						_Id;
};

template<class RunnableType> Poco::UInt32 PoolThread<RunnableType>::_Id = 0;



} // namespace Cumulus
