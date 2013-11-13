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
#include "Invoker.h"
#include "Publication.h"
#include "LUAListeners.h"
#include "LUAQualityOfService.h"

class LUAMyPublication;
class LUAPublicationBase {
public:
	static const char* Name;

	static void Clear(lua_State* pState,const Cumulus::Publication& publication);

protected:
	static Cumulus::Publication* Publication(Cumulus::Publication& publication) {return &publication;}
	static Cumulus::Publication* Publication(LUAMyPublication& luaPublication);

	static void Close(Cumulus::Publication& publication,const char* code=NULL,const char* description=NULL);
	static void Close(LUAMyPublication& luaPublication,const char* code=NULL,const char* description=NULL);
};

template<class PublicationType=Cumulus::Publication>
class LUAPublication : public LUAPublicationBase {
public:
	static int Get(lua_State *pState) {
		SCRIPT_CALLBACK(PublicationType,LUAPublication,publication)
			std::string name = SCRIPT_READ_STRING("");
			Cumulus::Publication* pPublication = Publication(publication);
			if(!pPublication) {
				SCRIPT_ERROR("Publication:%s, publication closed",name.c_str())
			} else {
				if(name=="publisherId") {
					SCRIPT_WRITE_NUMBER(pPublication->publisherId())
				} else if(name=="name") {
					SCRIPT_WRITE_STRING(pPublication->name().c_str())
				} else if(name=="listeners") {
					SCRIPT_WRITE_PERSISTENT_OBJECT(Cumulus::Listeners,LUAListeners,pPublication->listeners)
				} else if(name=="audioQOS") {
					SCRIPT_WRITE_PERSISTENT_OBJECT(Cumulus::QualityOfService,LUAQualityOfService,pPublication->audioQOS())
				} else if(name=="videoQOS") {
					SCRIPT_WRITE_PERSISTENT_OBJECT(Cumulus::QualityOfService,LUAQualityOfService,pPublication->videoQOS())
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
			}
		SCRIPT_CALLBACK_RETURN
	}

	static int Set(lua_State *pState) {
		SCRIPT_CALLBACK(PublicationType,LUAPublication,publication)
			lua_rawset(pState,1); // consumes key and value
		SCRIPT_CALLBACK_RETURN
	}

	static void ID(std::string& id){}
private:
	static int Close(lua_State *pState) {
		SCRIPT_CALLBACK(PublicationType,LUAPublication,publication)
			const char* code = SCRIPT_READ_STRING("");
		LUAPublicationBase::Close(publication,code,SCRIPT_READ_STRING(""));
		SCRIPT_CALLBACK_RETURN
	}

	static int	Flush(lua_State *pState) {
		SCRIPT_CALLBACK(PublicationType,LUAPublication,publication)
			Cumulus::Publication* pPublication = Publication(publication);
			if(!pPublication)
				SCRIPT_ERROR("Publication:flush, publication closed")
			else
				pPublication->flush();
		SCRIPT_CALLBACK_RETURN
	}

	static int	PushAudioPacket(lua_State *pState) {
		SCRIPT_CALLBACK(PublicationType,LUAPublication,publication)
			Cumulus::Publication* pPublication = Publication(publication);
			if(!pPublication) {
					SCRIPT_ERROR("Publication:pushAudioPacket, publication closed")
			} else {
				Poco::UInt32 time = SCRIPT_READ_UINT(0);
				SCRIPT_READ_BINARY(pData,size)
				if(pData) {
					Cumulus::PacketReader reader(pData,size);
					reader.next(SCRIPT_READ_UINT(0)); // offset
					pPublication->pushAudioPacket(time,reader,SCRIPT_READ_UINT(0));
				}
			}
		SCRIPT_CALLBACK_RETURN
	}

	static int PushVideoPacket(lua_State *pState) {
		SCRIPT_CALLBACK(PublicationType,LUAPublication,publication)
			Cumulus::Publication* pPublication = Publication(publication);
			if(!pPublication) {
				SCRIPT_ERROR("Publication:pushVideoPacket, publication closed")
			} else {
				Poco::UInt32 time = SCRIPT_READ_UINT(0);
				SCRIPT_READ_BINARY(pData,size)
				if(pData) {
					Cumulus::PacketReader reader(pData,size);
					reader.next(SCRIPT_READ_UINT(0)); // offset
					pPublication->pushVideoPacket(time,reader,SCRIPT_READ_UINT(0));
				}
			}
		SCRIPT_CALLBACK_RETURN
	}

	static int	PushDataPacket(lua_State *pState) {
		SCRIPT_CALLBACK(PublicationType,LUAPublication,publication)
			Cumulus::Publication* pPublication = Publication(publication);
			if(!pPublication) {
					SCRIPT_ERROR("Publication:pushDataPacket, publication closed")
			} else {
				std::string name = SCRIPT_READ_STRING("");
				SCRIPT_READ_BINARY(pData,size)
				if(pData) {
					Cumulus::PacketReader reader(pData,size);
					reader.next(SCRIPT_READ_UINT(0)); // offset
					pPublication->pushDataPacket(name,reader);
				}
			}
		SCRIPT_CALLBACK_RETURN
	}
};

class LUAMyPublication : public LUAPublication<LUAMyPublication> {
public:
	LUAMyPublication(Cumulus::Publication& publication,Cumulus::Invoker& invoker) : closed(false),invoker(invoker),publication(publication) {}

	static int	Destroy(lua_State* pState);

	Cumulus::Publication&	publication;
	Cumulus::Invoker&		invoker;
	bool					closed;
};

