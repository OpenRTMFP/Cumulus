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

#include "TaskHandler.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

TaskHandler::TaskHandler():_pTask(NULL) {
}
TaskHandler::~TaskHandler() {
}

void TaskHandler::waitHandle(Task& task) {
	ScopedLock<FastMutex> lockWait(_mutexWait);
	{
		ScopedLock<FastMutex> lock(_mutex);
		_pTask = &task;
	}
	requestHandle();
	_pTask->sleep();

	ScopedLock<FastMutex> lock(_mutex);
	_pTask=NULL;
}

void TaskHandler::giveHandle() {
	ScopedLock<FastMutex> lock(_mutex);
	if(!_pTask)
		return;
	_pTask->handle();
	_pTask->wakeUp();

}


} // namespace Cumulus
