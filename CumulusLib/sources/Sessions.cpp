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

#include "Sessions.h"
#include "Logs.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Sessions::Sessions(Gateway& gateway):_nextId(1),_gateway(gateway),_oldCount(0) {
}

Sessions::~Sessions() {
	clear();
}

void Sessions::clear() {
	// delete sessions
	if(!_sessions.empty())
		WARN("sessions are deleting");
	Iterator it;
	for(it=_sessions.begin();it!=_sessions.end();++it)
		delete it->second;
	_sessions.clear();
}

Session* Sessions::add(Session* pSession) {

	if(pSession->id!=_nextId) {
		ERROR("Session can not be inserted, its id %u not egal to nextId %u",pSession->id,_nextId);
		return NULL;
	}
	
	_sessions[_nextId] = pSession;
	DEBUG("Session %u created",_nextId);

	do {
		++_nextId;
	} while(_nextId==0 && find(_nextId));

	return pSession;
}

void Sessions::remove(map<UInt32,Session*>::iterator it) {
	DEBUG("Session %u died",it->second->id);
	_gateway.destroySession(*it->second);
	delete it->second;
	_sessions.erase(it);
}


Session* Sessions::find(const Poco::UInt8* peerId) {
	map<UInt32,Session*>::iterator it;
	for(it=_sessions.begin();it!=_sessions.end();++it) {
		if(it->second->peer == peerId) {
			if(it->second->died) {
				remove(it);
				return NULL;
			}
			return it->second;
		}
	}
	return NULL;
}

Session* Sessions::find(UInt32 id) {
	map<UInt32,Session*>::iterator it = _sessions.find(id);
	if(it==_sessions.end())
		return NULL;
	if(it->second->died) {
		remove(it);
		return NULL;
	}
	return it->second;
}



void Sessions::manage() {
	map<UInt32,Session*>::iterator it= _sessions.begin();
	while(it!=end()) {
		it->second->manage();
		if(it->second->died) {
			remove(it++);
			continue;
		}
		++it;
	}
	if(_sessions.size()!=_oldCount) {
		INFO("%u clients",count());
		_oldCount=_sessions.size();
	}
}




} // namespace Cumulus
