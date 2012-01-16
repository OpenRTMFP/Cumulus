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

#pragma once

#include "Script.h"


class LUAByteWriter {
public:
	static const char* Name;
	
	static int Get(lua_State *pState);
	static int Set(lua_State *pState);

	static void	ID(std::string& id){}

private:
	static int	WriteBoolean(lua_State *pState);
	static int	WriteByte(lua_State *pState);
	static int	WriteBytes(lua_State *pState);
	static int	WriteDouble(lua_State *pState);
	static int	WriteFloat(lua_State *pState);
	static int	WriteInt(lua_State *pState);
	static int	WriteMultiByte(lua_State *pState);
	static int	WriteAMF(lua_State *pState);
	static int	WriteShort(lua_State *pState);
	static int	WriteUnsignedByte(lua_State *pState);
	static int	WriteUnsignedInt(lua_State *pState);
	static int	WriteUnsignedShort(lua_State *pState);
	static int	WriteUTF(lua_State *pState);
	static int	WriteUTFBytes(lua_State *pState);
};


