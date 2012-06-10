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

#include "LUAServers.h"
#include "Servers.h"
#include "LUAServer.h"

using namespace std;
using namespace Cumulus;

const char*		LUAServers::Name="LUAServers";

int LUAServers::Pairs(lua_State* pState) {
	SCRIPT_CALLBACK(Servers,LUAServers,servers)
		lua_getglobal(pState,"next");
		if(!lua_iscfunction(pState,-1))
			SCRIPT_ERROR("'next' should be a LUA function, it should not be overloaded")
		else {
			lua_newtable(pState);
			Servers::Iterator it;
			for(it=servers.begin();it!=servers.end();++it) {
				SCRIPT_WRITE_PERSISTENT_OBJECT(ServerConnection,LUAServer,*it->second)
				lua_setfield(pState,-2,it->first.c_str());
			}
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAServers::Broadcast(lua_State* pState) {
	SCRIPT_CALLBACK(Servers,LUAServers,servers)
		string handler(SCRIPT_READ_STRING(""));
		ServerMessage message;
		AMFWriter writer(message);
		SCRIPT_READ_AMF(writer)
		servers.broadcast(handler,message);
	SCRIPT_CALLBACK_RETURN
}

int LUAServers::Get(lua_State *pState) {
	SCRIPT_CALLBACK(Servers,LUAServers,servers)
		string name = SCRIPT_READ_STRING("");
		if(name=="pairs")
			SCRIPT_WRITE_FUNCTION(&LUAServers::Pairs)
		else if(name == "count")
			SCRIPT_WRITE_NUMBER(servers.count())
		else if(name == "broadcast")
			SCRIPT_WRITE_FUNCTION(&LUAServers::Broadcast)
		else if(name=="(") {
			ServerConnection* pServer = servers.find(SCRIPT_READ_STRING(""));
			if(pServer)
				SCRIPT_WRITE_PERSISTENT_OBJECT(ServerConnection,LUAServer,*pServer)
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAServers::Set(lua_State *pState) {
	SCRIPT_CALLBACK(Servers,LUAServers,servers)
		string name = SCRIPT_READ_STRING("");
		lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}

