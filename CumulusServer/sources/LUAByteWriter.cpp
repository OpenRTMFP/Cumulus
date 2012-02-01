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

#include "LUAByteWriter.h"
#include "AMFWriter.h"
#include "Poco/StreamConverter.h"


using namespace Cumulus;
using namespace Poco;

const char*		LUAByteWriter::Name="LUAByteWriter";

int LUAByteWriter::Get(lua_State *pState) {
	SCRIPT_CALLBACK(AMFWriter,LUAByteWriter,writer)
		SCRIPT_READ_STRING(name,"")
		if(name=="writeBoolean")
			SCRIPT_WRITE_FUNCTION(&LUAByteWriter::WriteBoolean)
		else if(name=="writeByte")
			SCRIPT_WRITE_FUNCTION(&LUAByteWriter::WriteByte)
		else if(name=="writeBytes")
			SCRIPT_WRITE_FUNCTION(&LUAByteWriter::WriteBytes)
		else if(name=="writeDouble")
			SCRIPT_WRITE_FUNCTION(&LUAByteWriter::WriteDouble)
		else if(name=="writeFloat")
			SCRIPT_WRITE_FUNCTION(&LUAByteWriter::WriteFloat)
		else if(name=="writeInt")
			SCRIPT_WRITE_FUNCTION(&LUAByteWriter::WriteInt)
		else if(name=="writeMultiByte")
			SCRIPT_WRITE_FUNCTION(&LUAByteWriter::WriteMultiByte)
		else if(name=="writeAMF")
			SCRIPT_WRITE_FUNCTION(&LUAByteWriter::WriteAMF)
		else if(name=="writeShort")
			SCRIPT_WRITE_FUNCTION(&LUAByteWriter::WriteShort)
		else if(name=="writeUnsignedByte")
			SCRIPT_WRITE_FUNCTION(&LUAByteWriter::WriteUnsignedByte)
		else if(name=="writeUnsignedInt")
			SCRIPT_WRITE_FUNCTION(&LUAByteWriter::WriteUnsignedInt)
		else if(name=="writeUnsignedShort")
			SCRIPT_WRITE_FUNCTION(&LUAByteWriter::WriteUnsignedShort)
		else if(name=="writeUTF")
			SCRIPT_WRITE_FUNCTION(&LUAByteWriter::WriteUTF)
		else if(name=="writeUTFBytes")
			SCRIPT_WRITE_FUNCTION(&LUAByteWriter::WriteUTFBytes)
	SCRIPT_CALLBACK_RETURN
}

int LUAByteWriter::Set(lua_State *pState) {
	SCRIPT_CALLBACK(AMFWriter,LUAByteWriter,writer)
		SCRIPT_READ_STRING(name,"")
		lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteWriter::WriteBoolean(lua_State *pState) {
	SCRIPT_CALLBACK(AMFWriter,LUAByteWriter,writer)
		SCRIPT_READ_BOOL(value,false)
		writer.writer.write8(value ? 1 : 0);
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteWriter::WriteByte(lua_State *pState) {
	SCRIPT_CALLBACK(AMFWriter,LUAByteWriter,writer)
		SCRIPT_READ_INT(value,0)
		writer.writer.write8((UInt8)value);
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteWriter::WriteBytes(lua_State *pState) {
	SCRIPT_CALLBACK(AMFWriter,LUAByteWriter,writer)
		SCRIPT_READ_BINARY(value,size)
		writer.writer.writeRaw(value,size);
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteWriter::WriteDouble(lua_State *pState) {
	SCRIPT_CALLBACK(AMFWriter,LUAByteWriter,writer)
		SCRIPT_READ_DOUBLE(value,0)
		writer.writer << value;
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteWriter::WriteFloat(lua_State *pState) {
	SCRIPT_CALLBACK(AMFWriter,LUAByteWriter,writer)
		SCRIPT_READ_DOUBLE(value,0)
		writer.writer << (float)value;
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteWriter::WriteInt(lua_State *pState) {
	SCRIPT_CALLBACK(AMFWriter,LUAByteWriter,writer)
		SCRIPT_READ_INT(value,0)
		writer.writer.write32(value);
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteWriter::WriteMultiByte(lua_State *pState) {
	SCRIPT_CALLBACK(AMFWriter,LUAByteWriter,writer)
		SCRIPT_READ_STRING(data,"")
		SCRIPT_READ_STRING(format,"")
		TextEncoding& encodingFrom = TextEncoding::byName("UTF8");
		TextEncoding& encodingTo = TextEncoding::byName(format);
		OutputStreamConverter(writer.writer.stream(),encodingFrom,encodingTo).write(data.c_str(),data.size());
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteWriter::WriteAMF(lua_State *pState) {
	SCRIPT_CALLBACK(AMFWriter,LUAByteWriter,writer)
		SCRIPT_READ_AMF(writer)
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteWriter::WriteShort(lua_State *pState) {
	SCRIPT_CALLBACK(AMFWriter,LUAByteWriter,writer)
		SCRIPT_READ_INT(value,0)
		writer.writer.write16(value);
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteWriter::WriteUnsignedByte(lua_State *pState) {
	SCRIPT_CALLBACK(AMFWriter,LUAByteWriter,writer)
		SCRIPT_READ_UINT(value,0)
		writer.writer.write8(value);
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteWriter::WriteUnsignedInt(lua_State *pState) {
	SCRIPT_CALLBACK(AMFWriter,LUAByteWriter,writer)
		SCRIPT_READ_UINT(value,0)
		writer.writer.write32(value);
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteWriter::WriteUnsignedShort(lua_State *pState) {
	SCRIPT_CALLBACK(AMFWriter,LUAByteWriter,writer)
		SCRIPT_READ_UINT(value,0)
		writer.writer.write16(value);
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteWriter::WriteUTF(lua_State *pState) {
	SCRIPT_CALLBACK(AMFWriter,LUAByteWriter,writer)
		SCRIPT_READ_STRING(value,"")
		writer.writer.writeString16(value);
	SCRIPT_CALLBACK_RETURN
}

int	LUAByteWriter::WriteUTFBytes(lua_State *pState) {
	SCRIPT_CALLBACK(AMFWriter,LUAByteWriter,writer)
		SCRIPT_READ_STRING(value,"")
		writer.writer.writeRaw(value);
	SCRIPT_CALLBACK_RETURN
}
