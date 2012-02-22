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

#include "LUAInvoker.h"
#include "LUAClients.h"
#include "LUAEdges.h"
#include "LUAPublication.h"
#include "LUAPublications.h"
#include "LUAGroups.h"
#include "LUATCPClient.h"
#include "Server.h"
#include "Poco/Net/StreamSocket.h"
#include "math.h"

using namespace Cumulus;
using namespace Poco;
using namespace std;

const char*		LUAInvoker::Name="Cumulus::Invoker";

int	LUAInvoker::Publish(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		SCRIPT_READ_STRING(name,"")
		try {
			SCRIPT_WRITE_PERSISTENT_OBJECT(Publication,LUAPublication,invoker.publish(name))
			lua_getmetatable(pState,-1);
			lua_pushlightuserdata(pState,&invoker);
			lua_setfield(pState,-2,"__invoker");
			lua_pop(pState,1);
		} catch(Exception& ex) {
			SCRIPT_ERROR("%s",ex.displayText().c_str())
			SCRIPT_WRITE_NIL
		}
	SCRIPT_CALLBACK_RETURN
}

int	LUAInvoker::AbsolutePath(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		SCRIPT_READ_STRING(path,"")
		SCRIPT_WRITE_STRING((Server::WWWPath+path+"/").c_str())
	SCRIPT_CALLBACK_RETURN
}

int	LUAInvoker::CreateTCPClient(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		LUATCPClient::Create(((Server&)invoker).socketManager,pState);
	SCRIPT_CALLBACK_RETURN
}

int	LUAInvoker::ToAMF(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		BinaryStream stream;
		Cumulus::BinaryWriter rawWriter(stream);
		AMFWriter writer(rawWriter);
		SCRIPT_READ_AMF(writer)
		UInt32 size = stream.size();
		char* temp = new char[size]();
		stream.read(temp,size);
		SCRIPT_WRITE_BINARY(temp,size)
		delete [] temp;
	SCRIPT_CALLBACK_RETURN
}

int	LUAInvoker::ToAMF0(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		BinaryStream stream;
		Cumulus::BinaryWriter rawWriter(stream);
		AMFWriter writer(rawWriter);
		writer.amf0Preference=true;
		SCRIPT_READ_AMF(writer)
		UInt32 size = stream.size();
		char* temp = new char[size]();
		stream.read(temp,size);
		SCRIPT_WRITE_BINARY(temp,size)
		delete [] temp;
	SCRIPT_CALLBACK_RETURN
}

int	LUAInvoker::FromAMF(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		SCRIPT_READ_BINARY(data,size)
		PacketReader packet(data,size);
		AMFReader reader(packet);
		SCRIPT_WRITE_AMF(reader,0)
	SCRIPT_CALLBACK_RETURN
}


int LUAInvoker::Get(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		SCRIPT_READ_STRING(name,"")
		if(name=="clients") {
			SCRIPT_WRITE_OBJECT(Entities<Client>,LUAClients,invoker.clients)
		} else if(name=="groups") {
			SCRIPT_WRITE_OBJECT(Entities<Group>,LUAGroups,invoker.groups)
		} else if(name=="publications") {
			SCRIPT_WRITE_OBJECT(Publications,LUAPublications,invoker.publications)
		} else if(name=="publish") {
			SCRIPT_WRITE_FUNCTION(&LUAInvoker::Publish)
		} else if(name=="toAMF") {
			SCRIPT_WRITE_FUNCTION(&LUAInvoker::ToAMF)
		} else if(name=="toAMF0") {
			SCRIPT_WRITE_FUNCTION(&LUAInvoker::ToAMF0)
		} else if(name=="fromAMF") {
			SCRIPT_WRITE_FUNCTION(&LUAInvoker::FromAMF)
		} else if(name=="absolutePath") {
			SCRIPT_WRITE_FUNCTION(&LUAInvoker::AbsolutePath)
		} else if(name=="epochTime") {
			SCRIPT_WRITE_NUMBER(ROUND(Timestamp().epochMicroseconds()/1000))
		} else if(name=="edges") {
			SCRIPT_WRITE_OBJECT(Edges,LUAEdges,invoker.edges)
		} else if(name=="createTCPClient") {
			SCRIPT_WRITE_FUNCTION(&LUAInvoker::CreateTCPClient)
		} else if(name=="configs") {
			lua_getglobal(pState,"cumulus.configs");
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAInvoker::Set(lua_State *pState) {
	SCRIPT_CALLBACK(Invoker,LUAInvoker,invoker)
		SCRIPT_READ_STRING(name,"")
		lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}

