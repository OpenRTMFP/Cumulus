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

#include "LUAListener.h"
#include "LUAQualityOfService.h"
#include "LUAPublication.h"

using namespace Cumulus;

const char*		LUAListener::Name="Cumulus::Listener";

void LUAListener::Clear(lua_State* pState,const Listener& listener){
	Script::ClearPersistentObject<QualityOfService,LUAQualityOfService>(pState,listener.audioQOS());
	Script::ClearPersistentObject<QualityOfService,LUAQualityOfService>(pState,listener.videoQOS());
	Script::ClearPersistentObject<Listener,LUAListener>(pState,listener);
}

int LUAListener::Get(lua_State *pState) {
	SCRIPT_CALLBACK(Listener,LUAListener,listener)
		SCRIPT_READ_STRING(name,"")
		if(name=="id") {
			SCRIPT_WRITE_NUMBER(listener.id)
		} else if(name=="audioQOS") {
			SCRIPT_WRITE_PERSISTENT_OBJECT(QualityOfService,LUAQualityOfService,listener.audioQOS())
		} else if(name=="videoQOS") {
			SCRIPT_WRITE_PERSISTENT_OBJECT(QualityOfService,LUAQualityOfService,listener.videoQOS())
		} else if(name=="publication") {
			SCRIPT_WRITE_PERSISTENT_OBJECT(Publication,LUAPublication,listener.publication);
		} else if(name=="audioSampleAccess") {
			SCRIPT_WRITE_BOOL(listener.audioSampleAccess);
		} else if(name=="videoSampleAccess") {
			SCRIPT_WRITE_BOOL(listener.videoSampleAccess);
		} else if(name=="receiveAudio") {
			SCRIPT_WRITE_BOOL(listener.receiveAudio);
		} else if(name=="receiveVideo") {
			SCRIPT_WRITE_BOOL(listener.receiveVideo);
		}
	SCRIPT_CALLBACK_RETURN
}

int LUAListener::Set(lua_State *pState) {
	SCRIPT_CALLBACK(Listener,LUAListener,listener)
		SCRIPT_READ_STRING(name,"")
		if(name=="audioSampleAccess") {
			bool value = lua_toboolean(pState,-1)==0 ? false : true;
			if(value!=listener.audioSampleAccess)
				listener.sampleAccess(value,listener.videoSampleAccess);
		} else if(name=="videoSampleAccess") {
			bool value = lua_toboolean(pState,-1)==0 ? false : true;
			if(value!=listener.videoSampleAccess)
				listener.sampleAccess(listener.audioSampleAccess,value);
		} else if(name=="receiveAudio")
			listener.receiveAudio = lua_toboolean(pState,-1)==0 ? false : true;
		 else if(name=="receiveVideo")
			listener.receiveVideo = lua_toboolean(pState,-1)==0 ? false : true;
		else
			lua_rawset(pState,1); // consumes key and value
	SCRIPT_CALLBACK_RETURN
}
