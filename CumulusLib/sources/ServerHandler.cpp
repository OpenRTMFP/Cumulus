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

#include "ServerHandler.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

ServerHandler::ServerHandler(UInt8 keepAliveServer,UInt8 keepAlivePeer,ClientHandler* pClientHandler) :
	keepAliveServer(keepAliveServer<5 ? 5000 : keepAliveServer*1000),
	keepAlivePeer(keepAlivePeer<5 ? 5000 : keepAlivePeer*1000),
	_pClientHandler(pClientHandler) {
	
}


ServerHandler::~ServerHandler() {
	// delete groups
	list<Group*>::const_iterator it;
	for(it=_groups.begin();it!=_groups.end();++it)
		delete (*it);
	_groups.clear();
}


Group& ServerHandler::group(const vector<UInt8>& id) {
	Group* pGroup;
	list<Group*>::iterator it=((ServerHandler*)this)->_groups.begin();
	while(it!=_groups.end()) {
		pGroup=*it;
		if(pGroup->operator==(id))
			return *pGroup;
		// delete a possible empty group in same time
		if(pGroup->empty()) {
			delete pGroup;
			_groups.erase(it++);
			continue;
		}
		++it;
	}
	pGroup = new Group(id);
	((ServerHandler*)this)->_groups.push_back(pGroup);
	return *pGroup;
}

bool ServerHandler::connection(Peer& peer) {
	if(_pClientHandler)
		return _pClientHandler->onConnection(peer);
	return true;
}

void ServerHandler::failed(Peer& peer,const string& msg) {
	if(_pClientHandler)
		_pClientHandler->onFailed(peer,msg);
}

void ServerHandler::disconnection(Peer& peer) {
	if(_pClientHandler)
		_pClientHandler->onDisconnection(peer);
}



} // namespace Cumulus
