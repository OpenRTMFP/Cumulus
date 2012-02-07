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

#include "LUAFlowWriter.h"
#include "FlowWriter.h"
#include "Service.h"

using namespace std;
using namespace Cumulus;
using namespace Poco;

class NewWriter : public FlowWriter {
public:
	NewWriter(const string& signature,BandWriter& band) : FlowWriter(signature,band),pState(NULL) {
	}
	virtual ~NewWriter(){
		Script::ClearPersistentObject<NewWriter,LUAFlowWriter>(pState,*this);
	}
	void manage(Invoker& invoker){
		SCRIPT_BEGIN(pState)
			SCRIPT_MEMBER_FUNCTION_BEGIN(NewWriter,LUAFlowWriter,*this,"onManage")
				SCRIPT_FUNCTION_CALL
			SCRIPT_FUNCTION_END
		SCRIPT_END
		FlowWriter::manage(invoker);
	}
	lua_State*			pState;
};

const char*		LUAFlowWriter::Name="Cumulus::FlowWriter";


int LUAFlowWriter::Destroy(lua_State* pState) {
	SCRIPT_DESTRUCTOR_CALLBACK(NewWriter,LUAFlowWriter,writer)
		writer.close();
	SCRIPT_CALLBACK_RETURN
}

int LUAFlowWriter::Close(lua_State* pState) {
	SCRIPT_CALLBACK(NewWriter,LUAFlowWriter,writer)
		writer.close();
	SCRIPT_CALLBACK_RETURN
}


int LUAFlowWriter::Get(lua_State *pState) {
	SCRIPT_CALLBACK(FlowWriter,LUAFlowWriter,writer)
		SCRIPT_READ_STRING(name,"")
		if(name=="flush") {
			SCRIPT_WRITE_FUNCTION(&LUAFlowWriter::Flush)
		} else if(name=="writeAMFResult") {
			SCRIPT_WRITE_FUNCTION(&LUAFlowWriter::WriteAMFResult)
		} else if(name=="writeAMFMessage") {
			SCRIPT_WRITE_FUNCTION(&LUAFlowWriter::WriteAMFMessage)
		} else if(name=="writeStatusResponse") {
			SCRIPT_WRITE_FUNCTION(&LUAFlowWriter::WriteStatusResponse)
		} else if(name=="newFlowWriter") {
			SCRIPT_WRITE_FUNCTION(&LUAFlowWriter::NewFlowWriter)
		} else if(name=="close") {
			NewWriter* pNewWriter = dynamic_cast<NewWriter*>(&writer);
			if(pNewWriter)
				SCRIPT_WRITE_FUNCTION(&LUAFlowWriter::Close)
			else
				SCRIPT_ERROR("Impossible to close a flowWriter created by the client")
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAFlowWriter::Set(lua_State *pState) {
	SCRIPT_CALLBACK(FlowWriter,LUAFlowWriter,writer)
		SCRIPT_READ_STRING(name,"")
		lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}

int LUAFlowWriter::Flush(lua_State* pState) {
	SCRIPT_CALLBACK(FlowWriter,LUAFlowWriter,writer)
		SCRIPT_READ_BOOL(full,false)
		writer.flush(full);
	SCRIPT_CALLBACK_RETURN
}

int LUAFlowWriter::WriteAMFResult(lua_State* pState) {
	SCRIPT_CALLBACK(FlowWriter,LUAFlowWriter,writer)
		AMFWriter& amf = writer.writeAMFResult();
		SCRIPT_READ_AMF(amf)
	SCRIPT_CALLBACK_RETURN
}

int LUAFlowWriter::WriteAMFMessage(lua_State* pState) {
	SCRIPT_CALLBACK(FlowWriter,LUAFlowWriter,writer)
		SCRIPT_READ_STRING(name,"")
		AMFWriter& amf = writer.writeAMFMessage(name);
		SCRIPT_READ_AMF(amf)
	SCRIPT_CALLBACK_RETURN
}

int LUAFlowWriter::WriteStatusResponse(lua_State* pState) {
	SCRIPT_CALLBACK(FlowWriter,LUAFlowWriter,writer)
		SCRIPT_READ_STRING(code,"")
		SCRIPT_READ_STRING(description,"")
		AMFObjectWriter response = writer.writeStatusResponse(code,description);
		while(SCRIPT_CAN_READ) {
			SCRIPT_READ_STRING(name,"")
			response.writer.writePropertyName(name);
			Script::ReadAMF(pState,response.writer,1);
		}
	SCRIPT_CALLBACK_RETURN
}


int LUAFlowWriter::NewFlowWriter(lua_State* pState) {
	SCRIPT_CALLBACK(FlowWriter,LUAFlowWriter,writer)
		NewWriter& newWriter = writer.newFlowWriter<NewWriter>();
		newWriter.pState = pState;
		SCRIPT_WRITE_PERSISTENT_OBJECT(NewWriter,LUAFlowWriter,newWriter)
		SCRIPT_ADD_DESTRUCTOR(&LUAFlowWriter::Destroy)
	SCRIPT_CALLBACK_RETURN
}
