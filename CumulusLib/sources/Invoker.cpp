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

#include "Invoker.h"
#include "string.h"

using namespace std;
using namespace Poco;

namespace Cumulus {


Invoker::Invoker() : edges(_edges),clients(_clients),groups(_groups),edgesAttemptsBeforeFallback(0),udpBufferSize(0),_streams(_publications),publications(_publications),
	keepAliveServer(0),keepAlivePeer(0) {

}


Invoker::~Invoker() {
	// delete groups
	Entities<Group>::Iterator it;
	for(it=_groups.begin();it!=_groups.end();++it)
		delete it->second;
	// delete publications
	Publications::Iterator it2;
	for(it2=_publications.begin();it2!=_publications.end();++it2)
		delete it2->second;
}

Publication& Invoker::publish(const string& name) {
	UInt32 stream = _streams.create();
	try {
		memcpy((UInt8*)myself().id,id,ID_SIZE);
		Publication& publication = _streams.publish(myself(),stream,name);
		_publishers.insert(&publication);
		return publication;
	} catch(...) {
		_streams.destroy(stream);
		throw;
	}
}

void Invoker::unpublish(const Publication& publication) {
	if(_publishers.erase(&publication)==0) {
		ERROR("Publication %s already closed or you have not the handle on",publication.name().c_str())
		return;
	}
	UInt32 stream = publication.publisherId();
	memcpy((UInt8*)myself().id,id,ID_SIZE);
	_streams.unpublish(myself(),stream,publication.name());
	_streams.destroy(stream);
}



} // namespace Cumulus
