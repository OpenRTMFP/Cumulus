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

#include "LUAEdges.h"
#include "Edges.h"


using namespace Cumulus;

const char*		LUAEdges::Name="Cumulus::Edges";

int LUAEdges::Pairs(lua_State* pState) {
	SCRIPT_CALLBACK(Edges,LUAEdges,edges)
		lua_getglobal(pState,"next");
		if(!lua_iscfunction(pState,-1))
			SCRIPT_ERROR("'next' should be a LUA function, it should not be overloaded")
		else {
			lua_newtable(pState);
			Edges::Iterator it;
			for(it=edges.begin();it!=edges.end();++it) {
				SCRIPT_WRITE_NUMBER(it->second->count)
				lua_setfield(pState,-2,it->first.c_str());
			}
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAEdges::Get(lua_State *pState) {
	SCRIPT_CALLBACK(Edges,LUAEdges,edges)
		SCRIPT_READ_STRING(name,"")
		if(name=="pairs")
			SCRIPT_WRITE_FUNCTION(&LUAEdges::Pairs)
		else if(name=="count")
			SCRIPT_WRITE_NUMBER(edges.count())
		else if(name=="(") {
			SCRIPT_READ_STRING(address,"")
			Edge* pEdge = edges(address);
			if(pEdge)
				SCRIPT_WRITE_NUMBER(pEdge->count)
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAEdges::Set(lua_State *pState) {
	SCRIPT_CALLBACK(Edges,LUAEdges,edges)
		SCRIPT_READ_STRING(name,"")
		lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}

