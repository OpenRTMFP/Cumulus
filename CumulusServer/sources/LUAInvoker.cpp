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

#include "LUAInvoker.h"
#include "LUAClients.h"
#include "LUAEdges.h"
#include "LUAPublication.h"
#include "LUAPublications.h"
#include "LUAGroups.h"
#include "Server.h"

using namespace Cumulus;
using namespace Poco;
using namespace std;

const char*		LUAInvoker::Name="Cumulus::Invoker";

int	LUAInvoker::Publish(lua_State *pState) {
	SCRIPT_POP_CALLBACK(Invoker,LUAInvoker,invoker)
		SCRIPT_READ_STRING(name,"")
		try {
			SCRIPT_WRITE_PERSISTENT_OBJECT(Publication,LUAPublication,invoker.publish(name))
			lua_getmetatable(pState,-1);
			lua_pushlightuserdata(pState,&invoker);
			lua_setfield(pState,-2,"__invoker");
			lua_pop(pState,1);
		} catch(Exception& ex) {
			SCRIPT_ERROR("%s",ex.displayText().c_str())
			SCRIPT_WRITE_NIL
		}
	SCRIPT_CALLBACK_RETURN
}

int	LUAInvoker::AbsolutePath(lua_State *pState) {
	SCRIPT_POP_CALLBACK(Invoker,LUAInvoker,invoker)
		SCRIPT_READ_STRING(path,"")
		SCRIPT_WRITE_STRING((Server::WWWPath+path+"/").c_str())
	SCRIPT_CALLBACK_RETURN
}


int LUAInvoker::Get(lua_State *pState) {
	SCRIPT_PUSH_CALLBACK(Invoker,LUAInvoker,invoker)
		SCRIPT_READ_STRING(name,"")
		if(name=="clients") {
			SCRIPT_WRITE_OBJECT(Entities<Client>,LUAClients,invoker.clients)
		} else if(name=="groups") {
			SCRIPT_WRITE_OBJECT(Entities<Group>,LUAGroups,invoker.groups)
		} else if(name=="publications") {
			SCRIPT_WRITE_OBJECT(Publications,LUAPublications,invoker.publications)
		} else if(name=="publish") {
			SCRIPT_WRITE_FUNCTION(&LUAInvoker::Publish)
		} else if(name=="absolutePath") {
			SCRIPT_WRITE_FUNCTION(&LUAInvoker::AbsolutePath)
		} else if(name=="edges") {
			SCRIPT_WRITE_OBJECT(Edges,LUAEdges,invoker.edges)
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAInvoker::Set(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		SCRIPT_READ_STRING(name,"")

		if(name=="typeFactoryFunction") {
			if(lua_isfunction(pState,-1)) {
				if(lua_getmetatable(pState,LUA_GLOBALSINDEX)!=0) {
					lua_pushvalue(pState,-2);
					lua_setfield(pState,-2,"//typeFactory");
					lua_pop(pState,-2);
				} else
					SCRIPT_ERROR("No metatable on global table to store 'typeFactoryFunction' setting");
			} else
				SCRIPT_ERROR("'typeFactoryFunction' must be a function");
		} else
			lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}

