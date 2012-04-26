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
#include "Task.h"
#include "Poco/Mutex.h"
#include "Poco/Event.h"

namespace Cumulus {

class TaskHandler {
public:
	TaskHandler();
	~TaskHandler();

	void waitHandle(Task& task);

protected:
	void terminate();
	void giveHandle();
private:
	virtual void requestHandle()=0;

	Poco::FastMutex			_mutex;
	Poco::FastMutex			_mutexWait;
	Task*					_pTask;
	Poco::Event				_event;
	bool					_stop;
};


} // namespace Cumulus
