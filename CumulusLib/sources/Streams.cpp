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

Streams::Streams() : _nextId(0) {
	
}

Streams::~Streams() {
	// delete subscriptions
	PublicationIt it;
	for(it=_publications.begin();it!=_publications.end();++it)
		delete it->second;
}


Streams::PublicationIt Streams::publicationIt(const string& name) {
	// Return a suscription iterator and create the subscrition if it doesn't exist
	PublicationIt it = _publications.find(name);
	if(it != _publications.end())
		return it;
	return _publications.insert(pair<string,Publication*>(name,new Publication())).first;
}

void Streams::cleanPublication(PublicationIt& it) {
	// Delete susbscription is no more need
	if(it->second->count()==0 && it->second->publisherId==0) {
		delete it->second;
		_publications.erase(it);
	}
}

bool Streams::publish(UInt32 id,const string& name) {
	PublicationIt it = publicationIt(name);
	if(it->second->publisherId!=0)
		return false; // has already a publisher
	((UInt8&)it->second->publisherId) = id;
	return true;
}

void Streams::unpublish(UInt32 id,const string& name) {
	PublicationIt it = _publications.find(name);
	if(it == _publications.end()) {
		DEBUG("The stream '%s' with a %u id doesn't exist, unpublish useless",name.c_str(),id);
		return;
	}
	if(it->second->publisherId!=id) {
		WARN("Unpublish '%s' operation with a %u id different than its publisher %u id",name.c_str(),id,it->second->publisherId);
		return;
	}
	((UInt8&)it->second->publisherId) = 0;
	cleanPublication(it);
}

void Streams::subscribe(const string& name,Listener& listener) {
	publicationIt(name)->second->add(listener);
}

void Streams::unsubscribe(const string& name,Listener& listener) {
	PublicationIt it = _publications.find(name);
	if(it == _publications.end()) {
		DEBUG("The stream '%s' doesn't exists, unsubscribe useless",name.c_str());
		return;
	}
	it->second->remove(listener);
	cleanPublication(it);
}

Publication* Streams::publication(UInt32 id) {
	PublicationIt it;
	for(it=_publications.begin();it!=_publications.end();++it) {
		if(it->second->publisherId==id)
			return it->second;
	}
	return NULL;
}

UInt32 Streams::create() {
	while(!_streams.insert((++_nextId)==0 ? ++_nextId : _nextId).second);
	return _nextId;
}

void Streams::destroy(UInt32 id) {
	_streams.erase(id);
	PublicationIt it=_publications.begin();
	while(it!=_publications.end()) {
		if(it->second->publisherId==id) {
			((UInt8&)it->second->publisherId) = 0;
			PublicationIt it2(it++); // Causes a Unix std bug
			cleanPublication(it2);
		} else
			++it;
	}
}


} // namespace Cumulus
