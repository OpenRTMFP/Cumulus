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

#include "LUAPublication.h"
#include "LUAListeners.h"
#include "LUAQualityOfService.h"

using namespace std;
using namespace Cumulus;
using namespace Poco;

const char*		LUAPublication::Name="Cumulus::Publication";

void LUAPublication::Clear(lua_State* pState,const Publication& publication){
	Script::ClearPersistentObject<QualityOfService,LUAQualityOfService>(pState,publication.audioQOS());
	Script::ClearPersistentObject<QualityOfService,LUAQualityOfService>(pState,publication.videoQOS());
	Script::ClearPersistentObject<Listeners,LUAListeners>(pState,publication.listeners);
	Script::ClearPersistentObject<Publication,LUAPublication>(pState,publication);
}

int	LUAPublication::Close(lua_State *pState) {
	SCRIPT_CALLBACK(Publication,LUAPublication,publication)
		lua_getmetatable(pState,1);
		lua_getfield(pState,-1,"__invoker");
		lua_replace(pState,-2);
		if(lua_islightuserdata(pState,-1))
			((Invoker*)lua_touserdata(pState,-1))->unpublish(publication);
		else {
			string code = SCRIPT_READ_STRING("");
			publication.closePublisher(code,SCRIPT_READ_STRING(""));
		}
		lua_pop(pState,1);
	SCRIPT_CALLBACK_RETURN
}

int	LUAPublication::Flush(lua_State *pState) {
	SCRIPT_CALLBACK(Publication,LUAPublication,publication)
		publication.flush();
	SCRIPT_CALLBACK_RETURN
}

int	LUAPublication::PushAudioPacket(lua_State *pState) {
	SCRIPT_CALLBACK(Publication,LUAPublication,publication)
		UInt32 time = SCRIPT_READ_UINT(0);
		SCRIPT_READ_BINARY(pData,size)
		if(pData) {
			PacketReader reader(pData,size);
			reader.next(SCRIPT_READ_UINT(0)); // offset
			publication.pushAudioPacket(time,reader,SCRIPT_READ_UINT(0));
		}
	SCRIPT_CALLBACK_RETURN
}

int	LUAPublication::PushVideoPacket(lua_State *pState) {
	SCRIPT_CALLBACK(Publication,LUAPublication,publication)
		UInt32 time = SCRIPT_READ_UINT(0);
		SCRIPT_READ_BINARY(pData,size)
		if(pData) {
			PacketReader reader(pData,size);
			reader.next(SCRIPT_READ_UINT(0)); // offset
			publication.pushVideoPacket(time,reader,SCRIPT_READ_UINT(0));
		}
	SCRIPT_CALLBACK_RETURN
}

int	LUAPublication::PushDataPacket(lua_State *pState) {
	SCRIPT_CALLBACK(Publication,LUAPublication,publication)
		string name = SCRIPT_READ_STRING("");
		SCRIPT_READ_BINARY(pData,size)
		if(pData) {
			PacketReader reader(pData,size);
			reader.next(SCRIPT_READ_UINT(0)); // offset
			publication.pushDataPacket(name,reader);
		}
	SCRIPT_CALLBACK_RETURN
}


int LUAPublication::Get(lua_State *pState) {
	SCRIPT_CALLBACK(Publication,LUAPublication,publication)
		string name = SCRIPT_READ_STRING("");
		if(name=="publisherId") {
			SCRIPT_WRITE_NUMBER(publication.publisherId())
		} else if(name=="name") {
			SCRIPT_WRITE_STRING(publication.name().c_str())
		} else if(name=="listeners") {
			SCRIPT_WRITE_PERSISTENT_OBJECT(Listeners,LUAListeners,publication.listeners)
		} else if(name=="audioQOS") {
			SCRIPT_WRITE_PERSISTENT_OBJECT(QualityOfService,LUAQualityOfService,publication.audioQOS())
		} else if(name=="videoQOS") {
			SCRIPT_WRITE_PERSISTENT_OBJECT(QualityOfService,LUAQualityOfService,publication.videoQOS())
		} else if(name=="close") {
			SCRIPT_WRITE_FUNCTION(&LUAPublication::Close)
		} else if(name=="pushAudioPacket") {
			SCRIPT_WRITE_FUNCTION(&LUAPublication::PushAudioPacket)
		} else if(name=="flush") {
			SCRIPT_WRITE_FUNCTION(&LUAPublication::Flush)
		} else if(name=="pushVideoPacket") {
			SCRIPT_WRITE_FUNCTION(&LUAPublication::PushVideoPacket)
		} else if(name=="pushDataPacket") {
			SCRIPT_WRITE_FUNCTION(&LUAPublication::PushDataPacket)
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAPublication::Set(lua_State *pState) {
	SCRIPT_CALLBACK(Publication,LUAPublication,publication)
		string name = SCRIPT_READ_STRING("");
		lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}

