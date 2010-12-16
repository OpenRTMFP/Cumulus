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

#include "ServerData.h"

using namespace std;
using namespace Poco;
using namespace Poco::Data;

namespace Cumulus {

	ServerData::ServerData(UInt16 keepAlivePeer,UInt16 keepAliveServer) : keepAlivePeer(keepAlivePeer),keepAliveServer(keepAliveServer) {

	DataStream writer = _database.writer();
	
	writer << "CREATE TABLE IF NOT EXISTS routes (peer_id BLOB NOT NULL," \
				"route TEXT)",now;
	writer << "CREATE TABLE IF NOT EXISTS groups (peer_id BLOB NOT NULL," \
				"group_id BLOB NOT NULL)",now;
}

ServerData::~ServerData() {

}



void ServerData::addRoute(const BLOB& peerId,const string& route) {
	DataStream writer = _database.writer();
	writer << "INSERT INTO routes(peer_id,route) VALUES(?, ?)",
					use(peerId),use(route), now;
}

bool ServerData::addGroup(const BLOB& peerId,const BLOB& groupId,BLOB& peerOwner) {
	{
		DataStream reader = _database.reader();
		reader << "SELECT peer_id FROM groups WHERE group_id=?",
				use(groupId), into(peerOwner), limit(1),now;
		if(peerOwner.begin())
			return false;
	}
	DataStream writer = _database.writer();
	writer << "INSERT INTO groups(peer_id,group_id) VALUES(?, ?)",
					use(peerId),use(groupId), now;
	return true;
}

void ServerData::getRoutes(const BLOB& peerId,vector<string>& routes) {
	DataStream reader = _database.reader();
	reader << "SELECT route FROM routes WHERE peer_id=?",
			use(peerId), into(routes),now;
}



} // namespace Cumulus