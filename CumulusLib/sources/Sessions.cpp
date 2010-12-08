/* 
	Copyright 2010 cumulus.dev@gmail.com
 
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
#include "Poco/RandomStream.h"
#include "Poco/HexBinaryEncoder.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Sessions::Sessions() {

		
}

Sessions::~Sessions() {
	// delete sessions
	map<UInt32,Session*>::const_iterator it;
	for(it=_sessions.begin();it!=_sessions.end();++it)
		delete it->second;
	_sessions.clear();
}

Session* Sessions::add(Session* pSession) {
	Session* pSessionOther = find(pSession->id());
	if(pSessionOther && pSessionOther != pSession) {
		ERROR("A session exists yet with the same id '%u'",pSession->id());
		return NULL;
	}
	return _sessions[pSession->id()] = pSession;
}

Session* Sessions::find(const BLOB& peerId) {
	Iterator it;
	for(it=_sessions.begin();it!=_sessions.end();++it) {
		if(it->second->peerId()==peerId)
			return it->second;
	}
	char printPeerId[65];
	MemoryOutputStream mos(printPeerId,65);
	HexBinaryEncoder(mos).write((char*)peerId.begin(),32);
	mos.put('\0');
	WARN("Unknown session for a peerId of '%s'",printPeerId);
	return NULL;
}

Session* Sessions::find(UInt32 id) {
	if(_sessions.find(id)==_sessions.end())
		return NULL;
	return _sessions[id];
}


} // namespace Cumulus
