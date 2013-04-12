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

#include "LUAServer.h"

using namespace std;
using namespace Cumulus;

const char*		LUAServer::Name="LUAServer";

int LUAServer::Send(lua_State* pState) {
	SCRIPT_CALLBACK(ServerConnection,LUAServer,server)
		string handler(SCRIPT_READ_STRING(""));
		if(handler.empty() || handler==".") {
			ERROR("handler of one sending server message can't be null or equal to '.'")
		} else {
			ServerMessage message;
			AMFWriter writer(message);
			SCRIPT_READ_AMF(writer)
			server.send(handler,message);
		}
	SCRIPT_CALLBACK_RETURN
}


int LUAServer::Get(lua_State* pState) {
	SCRIPT_CALLBACK(ServerConnection,LUAServer,server)
		string name = SCRIPT_READ_STRING("");
		if(name=="send") {
			SCRIPT_WRITE_FUNCTION(&LUAServer::Send)
		} else if(name=="publicAddress") {
			SCRIPT_WRITE_STRING(server.publicAddress.c_str())
		} else if(name=="isTarget") {
			SCRIPT_WRITE_BOOL(server.isTarget)
		} else if(name=="address") {
			SCRIPT_WRITE_STRING(server.address.c_str())
		} else {
			std::map<string,string>::const_iterator it = 
server.properties.find(name);
			if(it!=server.properties.end())
				SCRIPT_WRITE_BINARY(it->second.c_str(),it->second.size())
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAServer::Set(lua_State* pState) {
	SCRIPT_CALLBACK(ServerConnection,LUAServer,server)
		string name = SCRIPT_READ_STRING("");
		lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}
