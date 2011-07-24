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

using namespace std;
using namespace Poco;

namespace Cumulus {

Startable::Startable(const string& name) : _name(name),_thread(name),_terminate(false) {
}

Startable::~Startable() {
	if(running())
		stop();
}

void Startable::start() {
	ScopedLock<FastMutex> lock(_mutex);

	if(running())
		return;

	_terminate = false;
	_thread.start(*this);
}


void Startable::run() {
	SetThreadName(_name.c_str());
	prerun();
}

bool Startable::prerun() {
	run(_terminate);
	return !_terminate;
}


void Startable::stop() {
	ScopedLock<FastMutex> lock(_mutex);
	if(!running())
		return;
	_terminate = true;
	// Attendre la fin!
	_thread.join();
}

} // namespace Cumulus
