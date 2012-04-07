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

#include "LUAMail.h"
#include "Service.h"

using namespace std;
using namespace Poco;


const char*		LUAMail::Name="LUAMail";

LUAMail::LUAMail(lua_State* pState) : _pState(pState) {
}

LUAMail::~LUAMail() {
}

void LUAMail::onSent(const char* error){
	SCRIPT_BEGIN(_pState)
		SCRIPT_MEMBER_FUNCTION_BEGIN(LUAMail,LUAMail,*this,"onSent")
			if(error)
				SCRIPT_WRITE_STRING(error)
			else
				SCRIPT_WRITE_NIL
			SCRIPT_FUNCTION_CALL
		SCRIPT_FUNCTION_END
	SCRIPT_END
	Script::ClearPersistentObject<LUAMail,LUAMail>(_pState,*this);
	delete this;
}

int LUAMail::Get(lua_State* pState) {
	SCRIPT_CALLBACK(LUAMail,LUAMail,mail)
		string name = SCRIPT_READ_STRING("");
	SCRIPT_CALLBACK_RETURN
}

int LUAMail::Set(lua_State* pState) {
	SCRIPT_CALLBACK(LUAMail,LUAMail,mail)
		string name = SCRIPT_READ_STRING("");
		lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}
