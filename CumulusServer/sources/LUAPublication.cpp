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

using namespace std;
using namespace Cumulus;

const char*		LUAPublicationBase::Name="Cumulus::Publication";

void LUAPublicationBase::Clear(lua_State* pState,const Cumulus::Publication& publication){
	Script::ClearPersistentObject<QualityOfService,LUAQualityOfService>(pState,publication.audioQOS());
	Script::ClearPersistentObject<QualityOfService,LUAQualityOfService>(pState,publication.videoQOS());
	Script::ClearPersistentObject<Listeners,LUAListeners>(pState,publication.listeners);
	Script::ClearPersistentObject<Cumulus::Publication,LUAPublication<>>(pState,publication);
}

Cumulus::Publication& LUAPublicationBase::Publication(LUAMyPublication& luaPublication) {
	return luaPublication.publication;
}

int LUAMyPublication::Destroy(lua_State* pState) {
	SCRIPT_DESTRUCTOR_CALLBACK(LUAMyPublication,LUAMyPublication,publication)
		publication.invoker.unpublish(publication.publication);
		delete &publication;
	SCRIPT_CALLBACK_RETURN
}

int LUAMyPublication::Close(lua_State *pState) {
	SCRIPT_CALLBACK(LUAMyPublication,LUAMyPublication,publication)
		publication.invoker.unpublish(publication.publication);
	SCRIPT_CALLBACK_RETURN
}

