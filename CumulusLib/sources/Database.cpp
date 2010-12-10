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

#include "Database.h"
#include "Logs.h"
#include "Poco/Data/SQLite/Connector.h"
#include "Poco/Data/BLOB.h"

using namespace std;
using namespace Poco;
using namespace Poco::Data;

namespace Cumulus {

RWLock Database::s_rwLock;
string Database::s_connector;
string Database::s_connectionString;

Database::Database() {
	
	if(!Loaded())
		Load(SQLite::Connector::KEY,"data.db");

	_pSession = new Session(s_connector,s_connectionString);

	data() << "CREATE TABLE IF NOT EXISTS routes (peer_id BLOB NOT NULL," \
				"route TEXT)",now;
	data() << "CREATE TABLE IF NOT EXISTS groups (peer_id BLOB NOT NULL," \
				"group_id BLOB NOT NULL)",now;
}

Database::~Database() {
	delete _pSession;
}

bool Database::Load(const string& connector,const string& connectionString) {
	if(connector==SQLite::Connector::KEY) {
		SQLite::Connector::registerConnector();
	} else {
		ERROR("Unknown database connector : %s",connector.c_str());
		return false;
	}
	s_connector = connector;
	s_connectionString = connectionString;
	return true;
}

void Database::Unload() {
	SQLite::Connector::unregisterConnector();
}

void Database::addRoute(const BLOB& peerId,const string& route) {
	//string host = format("%hu,
	ScopedWriteRWLock lock(s_rwLock);
	data() << "INSERT INTO routes(peer_id,route) VALUES(?, ?)",
					use(peerId),use(route), now;
}

bool Database::addGroup(const BLOB& peerId,const BLOB& groupId,BLOB& peerOwner) {
	{
		ScopedReadRWLock lock(s_rwLock);
		data() << "SELECT peer_id FROM groups WHERE group_id=?",
				use(groupId), into(peerOwner), limit(1),now;
		if(peerOwner.begin())
			return false;
	}
	ScopedWriteRWLock lock(s_rwLock);
	data() << "INSERT INTO groups(peer_id,group_id) VALUES(?, ?)",
					use(peerId),use(groupId), now;
	return true;
}

void Database::getRoutes(const BLOB& peerId,vector<string>& routes) {
	ScopedReadRWLock lock(s_rwLock);
	data() << "SELECT route FROM routes WHERE peer_id=?",
			use(peerId), into(routes),now;
}



} // namespace Cumulus