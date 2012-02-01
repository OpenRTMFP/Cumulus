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

#include "LUAListeners.h"
#include "Listeners.h"
#include "LUAListener.h"

using namespace Cumulus;

const char*		LUAListeners::Name="Cumulus::Listeners";

int LUAListeners::IPairs(lua_State* pState) {
	SCRIPT_CALLBACK(Listeners,LUAListeners,listeners)
		lua_getglobal(pState,"next");
		if(!lua_iscfunction(pState,-1))
			SCRIPT_ERROR("'next' should be a LUA function, it should not be overloaded")
		else {
			lua_newtable(pState);
			Listeners::Iterator it;
			for(it=listeners.begin();it!=listeners.end();++it) {
				SCRIPT_WRITE_PERSISTENT_OBJECT(Listener,LUAListener,*it->second)
				lua_rawseti(pState,-2,it->first);
			}
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAListeners::Get(lua_State *pState) {
	SCRIPT_CALLBACK(Listeners,LUAListeners,listeners)
		SCRIPT_READ_STRING(name,"")
		if(name=="ipairs")
			SCRIPT_WRITE_FUNCTION(&LUAListeners::IPairs)
		else if(name == "count")
			SCRIPT_WRITE_NUMBER(listeners.count())
	SCRIPT_CALLBACK_RETURN
}


int LUAListeners::Set(lua_State *pState) {
	SCRIPT_CALLBACK(Listeners,LUAListeners,listeners)
		SCRIPT_READ_STRING(name,"")
		lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}


