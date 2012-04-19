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

#include "SendingEngine.h"
#include "Logs.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

SendingEngine::SendingEngine(UInt32 numberOfThreads):_threads(numberOfThreads)  {
	vector<SendingThread*>::iterator it;
	for(UInt16 i=0;i<_threads.size();++i)
		_threads[i] = new SendingThread(i);
}

SendingEngine::~SendingEngine() {
	vector<SendingThread*>::iterator it;
	for(it=_threads.begin();it!=_threads.end();++it)
		delete *it;
}

void SendingEngine::clear() {
	vector<SendingThread*>::iterator it;
	for(it=_threads.begin();it!=_threads.end();++it)
		(*it)->clear();
}

SendingThread* SendingEngine::enqueue(AutoPtr<SendingUnit>& pSending,SendingThread* pThread) {

	if(!pSending->pAES) {
		ERROR("No aes encrypt engine set for this sending unit, impossible to send this packet");
		return pThread;
	}

	if(!pThread) {
		vector<SendingThread*>::const_iterator it;
		for(it=_threads.begin();it!=_threads.end();++it) {
			if(!pThread || pThread->size()<(*it)->size()) {
				pThread = *it;
				if(pThread->size()==0)
					break;
			}
		}
	}

	if (pThread->size() >= 10000) {
		WARN("Sending message refused on session %u (max queue reached), sending overload?",pSending->id);
		return pThread;
	}

	pThread->push(pSending);
	return pThread;
}


} // namespace Cumulus
