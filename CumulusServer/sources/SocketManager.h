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

#include "Startable.h"
#include "SocketManaged.h"
#include "Poco/Event.h"
#include <map>


class SocketManager : private Cumulus::Startable, private Poco::Net::SocketImpl {
public:
	SocketManager();
	virtual ~SocketManager();

	void add(SocketManaged& socket);
	void remove(SocketManaged& socket);
	bool realTime();

private:
	void run(const volatile bool& terminate);

	Poco::Mutex															_mutex;
	std::map<const Poco::Net::Socket,SocketManaged*>::iterator			_it;
	std::map<const Poco::Net::Socket,SocketManaged*>					_sockets;
	Poco::Timespan														_timeout;
	Poco::Event															_addEvent;
	bool																_stop;
};
