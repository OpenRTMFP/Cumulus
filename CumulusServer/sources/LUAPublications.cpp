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

#include "LUAPublications.h"
#include "Publications.h"
#include "LUAPublication.h"


using namespace Cumulus;

const char*		LUAPublications::Name="Cumulus::Publications";

int LUAPublications::Pairs(lua_State* pState) {
	SCRIPT_POP_CALLBACK(Publications,LUAPublications,publications)
		lua_getglobal(pState,"next");
		if(!lua_iscfunction(pState,-1))
			SCRIPT_ERROR("'next' should be a LUA function, it should not be overloaded")
		else {
			lua_newtable(pState);
			Publications::Iterator it;
			for(it=publications.begin();it!=publications.end();++it) {
				SCRIPT_WRITE_PERSISTENT_OBJECT(Publication,LUAPublication,*it->second)
				lua_setfield(pState,-2,it->first.c_str());
			}
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAPublications::Get(lua_State *pState) {
	SCRIPT_PUSH_CALLBACK(Publications,LUAPublications,publications)
		SCRIPT_READ_STRING(name,"")
		if(name=="pairs")
			SCRIPT_WRITE_FUNCTION(&LUAPublications::Pairs)
		else if(name == "count")
			SCRIPT_WRITE_NUMBER(publications.count())
		else if(name=="(") {
			SCRIPT_READ_STRING(publication,"")
			Publications::Iterator it = publications(publication);
			if(it!=publications.end())
				SCRIPT_WRITE_PERSISTENT_OBJECT(Publication,LUAPublication,*it->second)
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAPublications::Set(lua_State *pState) {
	SCRIPT_CALLBACK(Publications,LUAPublications,publications)
		SCRIPT_READ_STRING(name,"")
		lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}

