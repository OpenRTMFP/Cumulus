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
}

Database::~Database() {
	delete _pSession;
}

DataStream Database::writer() {
	s_rwLock.writeLock();
	return DataStream(*_pSession,s_rwLock);
}

DataStream Database::reader() {
	s_rwLock.readLock();
	return DataStream(*_pSession,s_rwLock);
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
	if(s_connector==SQLite::Connector::KEY) {
		SQLite::Connector::unregisterConnector();
	}
}



} // namespace Cumulus