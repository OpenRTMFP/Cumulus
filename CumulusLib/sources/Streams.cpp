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

#include "Streams.h"
#include "Logs.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

Streams::Streams(Handler& handler) : _nextId(0),publications(handler) {
	
}

Streams::~Streams() {
	
}

bool Streams::publish(Client& client,UInt32 id,const string& name) {
	Publications::Iterator it = publications.create(name);
	return it->second->start(client,id);
}

void Streams::unpublish(Client& client,UInt32 id,const string& name) {
	Publications::Iterator it = publications(name);
	if(it == publications.end()) {
		DEBUG("The stream '%s' with a %u id doesn't exist, unpublish useless",name.c_str(),id);
		return;
	}
	Publication& publication(*it->second);
	publication.stop(client,id);
	if(publication.publisherId()==0 && publication.listeners.count()==0)
		publications.destroy(it);
}

void Streams::subscribe(Client& client,UInt32 id,const string& name,FlowWriter& writer,double start) {
	publications.create(name)->second->addListener(client,id,writer,start==-3000 ? true : false);
}

void Streams::unsubscribe(Client& client,UInt32 id,const string& name) {
	Publications::Iterator it = publications(name);
	if(it == publications.end()) {
		DEBUG("The stream '%s' doesn't exists, unsubscribe useless",name.c_str());
		return;
	}
	Publication& publication(*it->second);
	publication.removeListener(client,id);
	if(publication.publisherId()==0 && publication.listeners.count()==0)
		publications.destroy(it);
}


UInt32 Streams::create() {
	while(!_streams.insert((++_nextId)==0 ? ++_nextId : _nextId).second);
	DEBUG("New stream %u",_nextId);
	return _nextId;
}

void Streams::destroy(UInt32 id) {
	DEBUG("Stream %u deleted",id);
	_streams.erase(id);
}


} // namespace Cumulus
