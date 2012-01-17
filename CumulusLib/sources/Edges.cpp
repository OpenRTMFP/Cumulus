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

#include "Edges.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Edge::Edge() : _raise(0),count(0) {
	
}

Edge::~Edge() {
	map<UInt32,Session*>::const_iterator it;
	for(it=_sessions.begin();it!=_sessions.end();++it)
		(bool&)it->second->died = true;
}

void Edge::update() {
	_raise=0;
	_timeLastExchange.update();
}

Edge* Edges::operator()(const string& address) {
	Iterator it = _edges.find(address);
	if(it==_edges.end())
		return NULL;
	return it->second;
}


} // namespace Cumulus
