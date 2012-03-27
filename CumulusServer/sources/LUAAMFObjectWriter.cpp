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

#include "LUAAMFObjectWriter.h"
#include "AMFObjectWriter.h"

using namespace Cumulus;
using namespace std;

const char*		LUAAMFObjectWriter::Name="Cumulus::AMFObjectWriter";

int LUAAMFObjectWriter::Get(lua_State *pState) {
	SCRIPT_CALLBACK(AMFObjectWriter,LUAAMFObjectWriter,writer)
		string name = SCRIPT_READ_STRING("");
		if(name=="write")
			SCRIPT_WRITE_FUNCTION(&LUAAMFObjectWriter::Write)
	SCRIPT_CALLBACK_RETURN
}

int LUAAMFObjectWriter::Set(lua_State *pState) {
	SCRIPT_CALLBACK(AMFObjectWriter,LUAAMFObjectWriter,writer)
		string name = SCRIPT_READ_STRING("");
		lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}

int LUAAMFObjectWriter::Write(lua_State* pState) {
	SCRIPT_CALLBACK(AMFObjectWriter,LUAAMFObjectWriter,writer)
		string name = SCRIPT_READ_STRING("");
		if(SCRIPT_CAN_READ) {
			writer.writer.writePropertyName(name);
			Script::ReadAMF(pState,writer.writer,1);
		} else
			SCRIPT_ERROR("This function takes 2 parameters: name and value")
	SCRIPT_CALLBACK_RETURN
}
