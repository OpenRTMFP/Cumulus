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

#include "SendingThread.h"
#include "Logs.h"
#include "Poco/NumberFormatter.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {


SendingThread::SendingThread(UInt32 id) : Startable("SendingThread"+NumberFormatter::format(id))  {
	setPriority(Thread::PRIO_HIGH);
}
SendingThread::~SendingThread() {
	clear();
}

UInt32 SendingThread::size() {
	ScopedLock<FastMutex> lock(_mutex);
	return _sendings.size();
}

void SendingThread::push(AutoPtr<SendingUnit>& sending) {
	ScopedLock<FastMutex> lock(_mutex);
	_sendings.push_back(sending);
	start();
	wakeUp();
}

void SendingThread::clear() {
	stop();
}

void SendingThread::run() {

	for(;;) {

		WakeUpType wakeUpType = sleep(40000); // 40 sec of timeout
		
		for(;;) {
			SendingUnit* pSending;
			{
				ScopedLock<FastMutex> lock(_mutex);
				if(_sendings.empty()) {
					if(wakeUpType!=WAKEUP) // STOP or TIMEOUT
						return;
					break;
				}
				pSending = _sendings.front();
			}

			// send
			RTMFP::Encode(*pSending->pAES,pSending->packet);
			RTMFP::Pack(pSending->packet,pSending->farId);
			try {
				if(pSending->socket.sendTo(pSending->packet.begin(),(int)pSending->packet.length(),pSending->address)!=pSending->packet.length())
					ERROR("Socket sending error on session %u : all data were not sent",pSending->id);
			} catch(Exception& ex) {
				 WARN("Socket sending error on session %u : %s",pSending->id,ex.displayText().c_str());
			} catch(exception& ex) {
				 WARN("Socket sending error on session %u : %s",pSending->id,ex.what());
			} catch(...) {
				 WARN("Socket sending unknown error on session %u",pSending->id);
			}
			
			ScopedLock<FastMutex> lock(_mutex);
			_sendings.pop_front();
		}
	}
}


} // namespace Cumulus
