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

#include "LUATCPClient.h"
#include "Service.h"

using namespace Poco;


const char*		LUATCPClient::Name="LUATCPClient";

LUATCPClient::LUATCPClient(SocketManager& manager,lua_State* pState) : _pState(pState),TCPClient(manager) {
}

LUATCPClient::~LUATCPClient() {
	Script::ClearPersistentObject<LUATCPClient,LUATCPClient>(_pState,*this);
}

UInt32 LUATCPClient::onReception(const UInt8* data,UInt32 size){
	UInt32 gotten=0;
	SCRIPT_BEGIN(_pState)
		SCRIPT_MEMBER_FUNCTION_BEGIN(LUATCPClient,LUATCPClient,*this,"onReception")
			SCRIPT_WRITE_BINARY(data,size)
			SCRIPT_FUNCTION_CALL
			SCRIPT_READ_UINT(readen,0)
			gotten = readen;
		SCRIPT_FUNCTION_END
	SCRIPT_END
	return gotten;
}

void LUATCPClient::onDisconnection(){
	SCRIPT_BEGIN(_pState)
		SCRIPT_MEMBER_FUNCTION_BEGIN(LUATCPClient,LUATCPClient,*this,"onDisconnection")
			SCRIPT_FUNCTION_CALL
		SCRIPT_FUNCTION_END
	SCRIPT_END
}

void LUATCPClient::Create(SocketManager& manager,lua_State* pState) {
	SCRIPT_BEGIN(pState)
		SCRIPT_WRITE_OBJECT(LUATCPClient,LUATCPClient,*(new LUATCPClient(manager,pState)))
		SCRIPT_ADD_DESTRUCTOR(&LUATCPClient::Destroy);
	SCRIPT_END
}


int	LUATCPClient::Destroy(lua_State* pState) {
	SCRIPT_DESTRUCTOR_CALLBACK(LUATCPClient,LUATCPClient,client)
		delete &client;
	SCRIPT_CALLBACK_RETURN
}

int	LUATCPClient::Connect(lua_State* pState) {
	SCRIPT_CALLBACK(LUATCPClient,LUATCPClient,client)
		SCRIPT_READ_STRING(host,"")
		SCRIPT_READ_UINT(port,0)
		client.connect(host,port);
		if(client.error())
			SCRIPT_WRITE_STRING(client.error())
	SCRIPT_CALLBACK_RETURN
}

int	LUATCPClient::Disconnect(lua_State* pState) {
	SCRIPT_CALLBACK(LUATCPClient,LUATCPClient,client)
		client.disconnect();
	SCRIPT_CALLBACK_RETURN
}


int	LUATCPClient::Send(lua_State* pState) {
	SCRIPT_CALLBACK(LUATCPClient,LUATCPClient,client)
		SCRIPT_READ_BINARY(data,size)
		if(!client.connected()) {
			SCRIPT_ERROR("TCPClient not connected");
		} else {
			client.send(data,size);
			if(client.error())
				SCRIPT_WRITE_STRING(client.error())
		}
	SCRIPT_CALLBACK_RETURN
}

int LUATCPClient::Get(lua_State* pState) {
	SCRIPT_CALLBACK(LUATCPClient,LUATCPClient,client)
		SCRIPT_READ_STRING(name,"")
		if(name=="connect") {
			SCRIPT_WRITE_FUNCTION(&LUATCPClient::Connect)
		} else if(name=="disconnect") {
			SCRIPT_WRITE_FUNCTION(&LUATCPClient::Disconnect)
		} else if(name=="send") {
			SCRIPT_WRITE_FUNCTION(&LUATCPClient::Send)
		} else if(name=="address") {
			if(client.connected())
				SCRIPT_WRITE_STRING(client.address().toString().c_str())
			else
				SCRIPT_WRITE_NIL
		} else if(name=="peerAddress") {
			if(client.connected())
				SCRIPT_WRITE_STRING(client.peerAddress().toString().c_str())
			else
				SCRIPT_WRITE_NIL
		} else if(name=="error") {
			if(client.error())
				SCRIPT_WRITE_STRING(client.error())
			else
				SCRIPT_WRITE_NIL
		} else if(name=="connected")
			SCRIPT_WRITE_BOOL(client.connected())
	SCRIPT_CALLBACK_RETURN
}

int LUATCPClient::Set(lua_State* pState) {
	SCRIPT_CALLBACK(TCPClient,LUATCPClient,client)
		SCRIPT_READ_STRING(name,"")
		lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}
