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
#include "Poco/Environment.h"

namespace Cumulus {


template <class RunnableType>
class PoolThreads {
public:
	PoolThreads(Poco::UInt32 numberOfThreads=0):_threads(numberOfThreads==0 ? Poco::Environment::processorCount() : numberOfThreads)  {
		std::vector<PoolThread<RunnableType>* >::iterator it;
		for(Poco::UInt16 i=0;i<_threads.size();++i)
			_threads[i] = new PoolThread<RunnableType>();
	}

	PoolThreads::~PoolThreads() {
		std::vector<PoolThread<RunnableType>* >::iterator it;
		for(it=_threads.begin();it!=_threads.end();++it)
			delete *it;
	}

	void PoolThreads::clear() {
		std::vector<PoolThread<RunnableType>* >::iterator it;
		for(it=_threads.begin();it!=_threads.end();++it)
			(*it)->clear();
	}

	PoolThread<RunnableType>* enqueue(Poco::AutoPtr<RunnableType>& pRunnable,PoolThread<RunnableType>* pThread) {

		if(!pThread) {
			vector<PoolThread<RunnableType>* >::const_iterator it;
			for(it=_threads.begin();it!=_threads.end();++it) {
				if(!pThread || pThread->queue()<(*it)->queue()) {
					pThread = *it;
					if(pThread->queue()==0)
						break;
				}
			}
		}

		if (pThread->queue() >= 10000)
			throw Poco::Exception("PoolThreads 10000 limit runnable entries for every thread reached");

		pThread->push(pRunnable);
		return pThread;
	}

private:
	std::vector<PoolThread<RunnableType>* >	_threads;
	Poco::FastMutex							_mutex;
};


} // namespace Cumulus
