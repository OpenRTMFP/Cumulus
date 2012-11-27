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

#include "LUAByteReader.h"
#include "AMFReader.h"
#include "Poco/StreamConverter.h"

using namespace std;
using namespace Cumulus;
using namespace Poco;

const char*		LUAByteReader::Name="LUAByteReader";

int LUAByteReader::Get(lua_State *pState) {
	SCRIPT_CALLBACK(AMFReader,LUAByteReader,reader)
		string name = SCRIPT_READ_STRING("");
		if(name=="readBoolean")
			SCRIPT_WRITE_FUNCTION(&LUAByteReader::ReadBoolean)
		else if(name=="readByte")
			SCRIPT_WRITE_FUNCTION(&LUAByteReader::ReadByte)
		else if(name=="readBytes")
			SCRIPT_WRITE_FUNCTION(&LUAByteReader::ReadBytes)
		else if(name=="readDouble")
			SCRIPT_WRITE_FUNCTION(&LUAByteReader::ReadDouble)
		else if(name=="readFloat")
			SCRIPT_WRITE_FUNCTION(&LUAByteReader::ReadFloat)
		else if(name=="readInt")
			SCRIPT_WRITE_FUNCTION(&LUAByteReader::ReadInt)
		else if(name=="readMultiByte")
			SCRIPT_WRITE_FUNCTION(&LUAByteReader::ReadMultiByte)
		else if(name=="readAMF")
			SCRIPT_WRITE_FUNCTION(&LUAByteReader::ReadAMF)
		else if(name=="readShort")
			SCRIPT_WRITE_FUNCTION(&LUAByteReader::ReadShort)
		else if(name=="readUnsignedByte")
			SCRIPT_WRITE_FUNCTION(&LUAByteReader::ReadUnsignedByte)
		else if(name=="readUnsignedInt")
			SCRIPT_WRITE_FUNCTION(&LUAByteReader::ReadUnsignedInt)
		else if(name=="readUnsignedShort")
			SCRIPT_WRITE_FUNCTION(&LUAByteReader::ReadUnsignedShort)
		else if(name=="readUTF")
			SCRIPT_WRITE_FUNCTION(&LUAByteReader::ReadUTF)
		else if(name=="readUTFBytes")
			SCRIPT_WRITE_FUNCTION(&LUAByteReader::ReadUTFBytes)
	SCRIPT_CALLBACK_RETURN
}

int LUAByteReader::Set(lua_State *pState) {
	SCRIPT_CALLBACK(AMFReader,LUAByteReader,reader)
		string name = SCRIPT_READ_STRING("");
		lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteReader::ReadBoolean(lua_State *pState) {
	SCRIPT_CALLBACK(AMFReader,LUAByteReader,reader)
		SCRIPT_WRITE_BOOL(reader.reader.read8()==0 ? false : true);
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteReader::ReadByte(lua_State *pState) {
	SCRIPT_CALLBACK(AMFReader,LUAByteReader,reader)
		SCRIPT_WRITE_INT(reader.reader.read8())
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteReader::ReadBytes(lua_State *pState) {
	SCRIPT_CALLBACK(AMFReader,LUAByteReader,reader)
		string result;
		reader.reader.readRaw(SCRIPT_READ_UINT(0),result);
		SCRIPT_WRITE_BINARY(result.c_str(),result.size())
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteReader::ReadDouble(lua_State *pState) {
	SCRIPT_CALLBACK(AMFReader,LUAByteReader,reader)
		double result;
		reader.reader >> result;
		SCRIPT_WRITE_NUMBER(result)
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteReader::ReadFloat(lua_State *pState) {
	SCRIPT_CALLBACK(AMFReader,LUAByteReader,reader)
		float result;
		reader.reader >> result;
		SCRIPT_WRITE_NUMBER(result)
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteReader::ReadInt(lua_State *pState) {
	SCRIPT_CALLBACK(AMFReader,LUAByteReader,reader)
		SCRIPT_WRITE_INT(reader.reader.read32())
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteReader::ReadMultiByte(lua_State *pState) {
	SCRIPT_CALLBACK(AMFReader,LUAByteReader,reader)
		vector<UInt8> result(SCRIPT_READ_UINT(0));
		TextEncoding& encodingFrom = TextEncoding::byName(SCRIPT_READ_STRING(""));
		TextEncoding& encodingTo = TextEncoding::byName("UTF8");
		InputStreamConverter(reader.reader.stream(),encodingFrom,encodingTo).read((char*)&result[0],result.size());
		SCRIPT_WRITE_BINARY(&result[0],result.size())
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteReader::ReadAMF(lua_State *pState) {
	SCRIPT_CALLBACK(AMFReader,LUAByteReader,reader)
		SCRIPT_WRITE_AMF(reader,SCRIPT_READ_UINT(0))
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteReader::ReadShort(lua_State *pState) {
	SCRIPT_CALLBACK(AMFReader,LUAByteReader,reader)
		SCRIPT_WRITE_INT(reader.reader.read16())
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteReader::ReadUnsignedByte(lua_State *pState) {
	SCRIPT_CALLBACK(AMFReader,LUAByteReader,reader)
		SCRIPT_WRITE_NUMBER(reader.reader.read8())
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteReader::ReadUnsignedInt(lua_State *pState) {
	SCRIPT_CALLBACK(AMFReader,LUAByteReader,reader)
		SCRIPT_WRITE_NUMBER(reader.reader.read32())
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteReader::ReadUnsignedShort(lua_State *pState) {
	SCRIPT_CALLBACK(AMFReader,LUAByteReader,reader)
		SCRIPT_WRITE_NUMBER(reader.reader.read16())
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteReader::ReadUTF(lua_State *pState) {
	SCRIPT_CALLBACK(AMFReader,LUAByteReader,reader)
		string value;
		reader.reader.readString16(value);
		SCRIPT_WRITE_BINARY(value.c_str(),value.size())
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteReader::ReadUTFBytes(lua_State *pState) {
	SCRIPT_CALLBACK(AMFReader,LUAByteReader,reader)
		string value;
		reader.reader.readRaw(SCRIPT_READ_UINT(0),value);
		SCRIPT_WRITE_BINARY(value.c_str(),value.size())
	SCRIPT_CALLBACK_RETURN
}
