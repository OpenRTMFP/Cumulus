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

#include "Publications.h"


using namespace std;
using namespace Poco;


namespace Cumulus {

Publications::Publications(Handler& handler) : _handler(handler) {
	
}

Publications::~Publications() {
	// delete publications
	Iterator it;
	for(it=_publications.begin();it!=_publications.end();++it)
		delete it->second;
}

Publications::Iterator Publications::operator()(UInt32 id) {
	Iterator it;
	for(it=begin();it!=end();++it) {
		if(it->second->publisherId()==id)
			return it;
	}
	return end();
}

Publications::Iterator Publications::create(const string& name) {
	Iterator it = _publications.lower_bound(name);
	if(it!=end() && it->first==name)
		return it;
	if(it!=_publications.begin())
		--it;
	return _publications.insert(it,pair<string,Publication*>(name,new Publication(name,_handler)));
}

void Publications::destroy(const Iterator& it) {
	delete it->second;
	_publications.erase(it);
}


} // namespace Cumulus
