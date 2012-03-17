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

#include "LUAClient.h"
#include "Util.h"
#include "LUAFlowWriter.h"


using namespace std;
using namespace Cumulus;

const char*		LUAClient::Name="Cumulus::Client";

void LUAClient::Clear(lua_State* pState,const Client& client){
	Script::ClearPersistentObject<FlowWriter,LUAFlowWriter>(pState,((Client&)client).writer());
	Script::ClearPersistentObject<Client,LUAClient>(pState,client);
}

int LUAClient::Get(lua_State *pState) {
	SCRIPT_CALLBACK(Client,LUAClient,client)
		SCRIPT_READ_STRING(name,"")
		if(name=="writer") {
			SCRIPT_CALLBACK_NOTCONST_CHECK
			SCRIPT_WRITE_PERSISTENT_OBJECT(FlowWriter,LUAFlowWriter,client.writer())
		} else if(name=="id") {
			SCRIPT_WRITE_STRING(Util::FormatHex(client.id,ID_SIZE).c_str())
		} else if(name=="rawId") {
			SCRIPT_WRITE_BINARY(client.id,ID_SIZE);
		} else if(name=="path") {
			SCRIPT_WRITE_STRING(client.path.c_str())
		} else if(name=="address") {
			SCRIPT_WRITE_STRING(client.address.toString().c_str())
		} else if(name=="pageUrl") {
			SCRIPT_WRITE_STRING(client.pageUrl.toString().c_str())
		} else if(name=="close") {
			SCRIPT_WRITE_FUNCTION(&LUAClient::Close)
		} else if(name=="flashVersion") {
			SCRIPT_WRITE_STRING(client.flashVersion.c_str())
		} else if(name=="ping") {
			SCRIPT_WRITE_NUMBER(client.ping)
		} else if(name=="swfUrl") {
			SCRIPT_WRITE_STRING(client.swfUrl.toString().c_str())
		} else {
			map<string,string>::const_iterator it = client.properties.find(name);
			if(it!=client.properties.end())
				SCRIPT_WRITE_STRING(it->second.c_str())
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAClient::Set(lua_State *pState) {
	SCRIPT_CALLBACK(Client,LUAClient,client)
		SCRIPT_READ_STRING(name,"")
		lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}

int	LUAClient::Close(lua_State *pState) {
	SCRIPT_CALLBACK(Client,LUAClient,client)
		client.close();
	SCRIPT_CALLBACK_RETURN
}




