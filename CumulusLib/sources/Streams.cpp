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

Streams::Streams(map<string,Publication*>&	publications) : _nextId(0),_publications(publications) {
	
}

Streams::~Streams() {
	
}


Publication& Streams::publish(Peer& peer,UInt32 id,const string& name) {
	Publications::Iterator it = createPublication(name);
	Publication& publication(*it->second);
	string error;
	Publication::StartCode code = publication.start(peer,id,error);
	if(code) {
		if(publication.publisherId()==0 && publication.listeners.count()==0)
			destroyPublication(it);
		throw Exception(error,code);
	}
	return publication;
}

void Streams::unpublish(Peer& peer,UInt32 id,const string& name) {
	Publications::Iterator it = _publications.find(name);
	if(it == _publications.end()) {
		DEBUG("The stream '%s' with a %u id doesn't exist, unpublish useless",name.c_str(),id);
		return;
	}
	Publication& publication(*it->second);
	publication.stop(peer,id);
	if(publication.publisherId()==0 && publication.listeners.count()==0)
		destroyPublication(it);
}

bool Streams::subscribe(Peer& peer,UInt32 id,const string& name,FlowWriter& writer,double start) {
	Publications::Iterator it = createPublication(name);
	Publication& publication(*it->second);
	bool result = publication.addListener(peer,id,writer,start==-3000 ? true : false);
	if(!result && publication.publisherId()==0 && publication.listeners.count()==0)
		destroyPublication(it);
	return result;
}

void Streams::unsubscribe(Peer& peer,UInt32 id,const string& name) {
	Publications::Iterator it = _publications.find(name);
	if(it == _publications.end()) {
		DEBUG("The stream '%s' doesn't exists, unsubscribe useless",name.c_str());
		return;
	}
	Publication& publication(*it->second);
	publication.removeListener(peer,id);
	if(publication.publisherId()==0 && publication.listeners.count()==0)
		destroyPublication(it);
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

Publications::Iterator Streams::createPublication(const string& name) {
	Publications::Iterator it = _publications.lower_bound(name);
	if(it!=_publications.end() && it->first==name)
		return it;
	if(it!=_publications.begin())
		--it;
	return _publications.insert(it,pair<string,Publication*>(name,new Publication(name)));
}

void Streams::destroyPublication(const Publications::Iterator& it) {
	delete it->second;
	_publications.erase(it);
}


} // namespace Cumulus
