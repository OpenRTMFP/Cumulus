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
#include "Listener.h"
#include "Publications.h"
#include <set>

namespace Cumulus {

class Streams {
public:
	Streams(std::map<std::string,Publication*>&	publications);
	virtual ~Streams();

	Poco::UInt32	create();
	void			destroy(Poco::UInt32 id);

	Publication&			publish(Peer& peer,Poco::UInt32 id,const std::string& name);
	void					unpublish(Peer& peer,Poco::UInt32 id,const std::string& name);
	bool					subscribe(Peer& peer,Poco::UInt32 id,const std::string& name,FlowWriter& writer,double start=-2000);
	void					unsubscribe(Peer& peer,Poco::UInt32 id,const std::string& name);

private:
	Publications::Iterator	createPublication(const std::string& name);
	void					destroyPublication(const Publications::Iterator& it);

	std::map<std::string,Publication*>&	_publications;
	std::set<Poco::UInt32>				_streams;
	Poco::UInt32						_nextId;
};


} // namespace Cumulus
