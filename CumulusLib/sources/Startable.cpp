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


#include "Startable.h"
#include "Logs.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

StartableProcess::StartableProcess(Startable& startable):_startable(startable){
}

void StartableProcess::run() {
	_startable.prerun();
}

Startable::Startable(const string& name) : _name(name),_thread(name),_stop(true),_haveToJoin(false),_process(*this) {
}

Startable::~Startable() {
	if(running())
		WARN("Startable::stop should be called by the child class");
	stop();
}

void Startable::start() {
	ScopedLock<FastMutex> lock(_mutex);
	if(!_stop) // if running
		return;
	if(_haveToJoin) {
		_thread.join();
		_haveToJoin=false;
	}
	try {
		_thread.start(_process);
		_haveToJoin = true;
		ScopedLock<FastMutex> lock(_mutexStop);
		_stop=false;
	} catch (Poco::Exception& ex) {
		ERROR("Impossible to start the thread : %s",ex.displayText().c_str());
	}
}

void Startable::prerun() {
	run();
	ScopedLock<FastMutex> lock(_mutexStop);
	_stop=true;
}

Startable::WakeUpType Startable::sleep(UInt32 timeout) {
	if(_stop)
		return STOP;
	 WakeUpType result = WAKEUP;
	 if(timeout>0) {
		 if(!_wakeUpEvent.tryWait(timeout))
			 result = TIMEOUT;
	 } else
		 _wakeUpEvent.wait();
	if(_stop)
		return STOP;
	return result;
}

void Startable::stop() {
	ScopedLock<FastMutex> lock(_mutex);
	{
		ScopedLock<FastMutex> lock(_mutexStop);
		if(_stop) {
			if(_haveToJoin) {
				_thread.join();
				_haveToJoin=false;
			}
			return;
		}
		_stop=true;
	}
	_wakeUpEvent.set();
	// Attendre la fin!
	_thread.join();
	_haveToJoin=false;
}

} // namespace Cumulus
