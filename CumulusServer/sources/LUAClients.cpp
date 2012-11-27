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

#include "LUAClients.h"
#include "Invoker.h"
#include "LUAClient.h"
#include "Util.h"
#include "Poco/HexBinaryDecoder.h"

using namespace Cumulus;
using namespace Poco;
using namespace std;

const char*		LUAClients::Name="Cumulus::Entities<Client>";

int LUAClients::Pairs(lua_State* pState) {
	SCRIPT_CALLBACK(Entities<Client>,LUAClients,clients)
		lua_getglobal(pState,"next");
		if(!lua_iscfunction(pState,-1))
			SCRIPT_ERROR("'next' should be a LUA function, it should not be overloaded")
		else {
			lua_newtable(pState);
			Entities<Client>::Iterator it;
			for(it=clients.begin();it!=clients.end();++it) {
				SCRIPT_WRITE_PERSISTENT_OBJECT(Client,LUAClient,*it->second)
				lua_setfield(pState,-2,Cumulus::Util::FormatHex(it->second->id,ID_SIZE).c_str());
			}
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAClients::Get(lua_State *pState) {
	SCRIPT_CALLBACK(Entities<Client>,LUAClients,clients)
		string name = SCRIPT_READ_STRING("");
		if(name=="pairs")
			SCRIPT_WRITE_FUNCTION(&LUAClients::Pairs)
		else if(name=="count")
			SCRIPT_WRITE_NUMBER(clients.count())
		else if(name=="(") {
			SCRIPT_READ_BINARY(id,size)
			Client* pClient = NULL;
			if(size==ID_SIZE)
				pClient = clients(id);
			else if(size==(ID_SIZE*2)) {
				stringstream ss;
				ss.write((const char*)id,size);
				HexBinaryDecoder(ss).read((char*)id,ID_SIZE);
				pClient = clients(id);
			} else if(id)
				SCRIPT_ERROR("Bad client format id %s",Cumulus::Util::FormatHex(id,ID_SIZE).c_str())
			else
				SCRIPT_ERROR("Client id argument missing")
			if(pClient)
				SCRIPT_WRITE_PERSISTENT_OBJECT(Client,LUAClient,*pClient)
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAClients::Set(lua_State *pState) {
	SCRIPT_CALLBACK(Entities<Client>,LUAClients,clients)
		string name = SCRIPT_READ_STRING("");
		lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}
