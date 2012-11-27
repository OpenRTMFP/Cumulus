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

#include "Script.h"
#include "Logs.h"
#include "Util.h"
#include "LUAByteReader.h"
#include "LUAByteWriter.h"
#include "Service.h"
#include "Poco/DateTime.h"
#include "Poco/Timezone.h"
#include <math.h>
extern "C" {
	#include "lua5.1/lualib.h"
}

using namespace std;
using namespace Poco;
using namespace Cumulus;

lua_Debug	Script::LuaDebug;

const char* Script::LastError(lua_State *pState) {
	const char* error = lua_tostring(pState,-1);
	if(error)
		lua_pop(pState,1);
	return error;
}

int Script::Error(lua_State *pState) {
#undef SCRIPT_LOG_NAME_DISABLED
#define SCRIPT_LOG_NAME_DISABLED true
	SCRIPT_BEGIN(pState)
		string msg;
		SCRIPT_ERROR("%s",ToString(pState,msg));
	SCRIPT_END
	return 0;
}
int Script::Warn(lua_State *pState) {
#undef SCRIPT_LOG_NAME_DISABLED
#define SCRIPT_LOG_NAME_DISABLED true
	SCRIPT_BEGIN(pState)
		string msg;
		SCRIPT_WARN("%s",ToString(pState,msg));
	SCRIPT_END
	return 0;
}
int Script::Note(lua_State *pState) {
#undef SCRIPT_LOG_NAME_DISABLED
#define SCRIPT_LOG_NAME_DISABLED true
	SCRIPT_BEGIN(pState)
		string msg;
		SCRIPT_NOTE("%s",ToString(pState,msg));
	SCRIPT_END
	return 0;
}
int Script::Info(lua_State *pState) {
#undef SCRIPT_LOG_NAME_DISABLED
#define SCRIPT_LOG_NAME_DISABLED true
	SCRIPT_BEGIN(pState)
		string msg;
		SCRIPT_INFO("%s",ToString(pState,msg));
	SCRIPT_END
	return 0;
}
int Script::Debug(lua_State *pState) {
#undef SCRIPT_LOG_NAME_DISABLED
#define SCRIPT_LOG_NAME_DISABLED true
	SCRIPT_BEGIN(pState)
		string msg;
		SCRIPT_DEBUG("%s",ToString(pState,msg));
	SCRIPT_END
	return 0;
}
int Script::Trace(lua_State *pState) {
#undef SCRIPT_LOG_NAME_DISABLED
#define SCRIPT_LOG_NAME_DISABLED true
	SCRIPT_BEGIN(pState)
		string msg;
		SCRIPT_TRACE("%s",ToString(pState,msg));
	SCRIPT_END
	return 0;
}


int Script::Panic(lua_State *pState) {
	SCRIPT_BEGIN(pState)
		string msg;
		SCRIPT_FATAL("%s",ToString(pState,msg));
	SCRIPT_END
	return 1;
}

lua_State* Script::CreateState() {
	lua_State* pState = lua_open();
	luaL_openlibs(pState);
	lua_atpanic(pState,&Script::Panic);

	lua_pushcfunction(pState,&Script::Error);
	lua_setglobal(pState,"ERROR");
	lua_pushcfunction(pState,&Script::Warn);
	lua_setglobal(pState,"WARN");
	lua_pushcfunction(pState,&Script::Note);
	lua_setglobal(pState,"NOTE");
	lua_pushcfunction(pState,&Script::Info);
	lua_setglobal(pState,"INFO");
	lua_pushcfunction(pState,&Script::Debug);
	lua_setglobal(pState,"DEBUG");
	lua_pushcfunction(pState,&Script::Trace);
	lua_setglobal(pState,"TRACE");

	return pState;
}

void Script::CloseState(lua_State* pState) {
	if(pState)
		lua_close(pState);
}


void Script::Test(lua_State* pState) {
    int i; 
    int top = lua_gettop(pState);
 
    printf("total in stack %d\n",top);
 
    for (i = 1; i <= top; i++)
    {  /* repeat for each level */
        int t = lua_type(pState, i);
        switch (t) {
            case LUA_TSTRING:  /* strings */
                printf("string: '%s'\n", lua_tostring(pState, i));
                break;
            case LUA_TBOOLEAN:  /* booleans */
                printf("boolean %s\n",lua_toboolean(pState, i) ? "true" : "false");
                break;
            case LUA_TNUMBER:  /* numbers */
                printf("number: %g\n", lua_tonumber(pState, i));
                break;
            default:  /* other values */
                printf("%s\n", lua_typename(pState, t));
                break;
        }
        printf("  ");  /* put a separator */
    }
    printf("\n");  /* end the listing */

}


void Script::WriteAMF(lua_State *pState,AMFReader& reader,UInt32 count) {
	AMF::Type type;
	bool all=count==0;
	if(all)
		count=1;
	while(count>0 && (type = reader.followingType())!=AMF::End) {
		WriteAMF(pState,type,reader);
		if(!all)
			--count;
	}
}


void Script::WriteAMF(lua_State *pState,AMF::Type type,AMFReader& reader) {
	SCRIPT_BEGIN(pState)
	switch(type) {
		case AMF::Null:
			reader.readNull();
			lua_pushnil(pState);
			break;
		case AMF::Boolean:
			lua_pushboolean(pState,reader.readBoolean());
			break;
		case AMF::Integer:
			lua_pushinteger(pState,reader.readInteger());
			break;
		case AMF::Number:
			lua_pushnumber(pState,reader.readNumber());
			break;
		case AMF::String: {
			string value;
			reader.read(value);
			lua_pushlstring(pState,value.c_str(),value.size());
			break;
		}
		case AMF::Date: {
			Timestamp time = reader.readDate();
			DateTime date(time);
			lua_newtable(pState);
			lua_pushnumber(pState,(double)(time.epochMicroseconds()/1000));
			lua_setfield(pState,-2,"__time");
			lua_pushnumber(pState,date.year());
			lua_setfield(pState,-2,"year");
			lua_pushnumber(pState,date.month());
			lua_setfield(pState,-2,"month");
			lua_pushnumber(pState,date.day());
			lua_setfield(pState,-2,"day");
			lua_pushnumber(pState,date.dayOfYear());
			lua_setfield(pState,-2,"yday");
			lua_pushnumber(pState,date.dayOfWeek());
			lua_setfield(pState,-2,"wday");
			lua_pushnumber(pState,date.hour());
			lua_setfield(pState,-2,"hour");
			lua_pushnumber(pState,date.minute());
			lua_setfield(pState,-2,"min");
			lua_pushnumber(pState,date.second());
			lua_setfield(pState,-2,"sec");
			lua_pushnumber(pState,date.millisecond());
			lua_setfield(pState,-2,"msec");
			lua_pushboolean(pState,Timezone::isDst(time));
			lua_setfield(pState,-2,"isdst");
			break;
		}
		case AMF::Array: {
			if(reader.readArray()) {
				lua_newtable(pState);
				UInt32 index=0;
				string name;
				while((type=reader.readItem(name))!=AMF::End) {
					WriteAMF(pState,type,reader);
					if(name.empty())
						lua_rawseti(pState,-2,++index);
					else
						lua_setfield(pState,-2,name.c_str());
				}
			}
			break;
		}
		case AMF::Dictionary: {
			bool weakKeys=false;
			if(reader.readDictionary(weakKeys)) {
				lua_newtable(pState);
				if(weakKeys) {
					lua_newtable(pState);
					lua_pushstring(pState,"k");
					lua_setfield(pState,-2,"__mode");
					lua_setmetatable(pState,-2);
				}
				string name;
				UInt32 size=0;
				while((type=reader.readKey())!=AMF::End) {
					WriteAMF(pState,type,reader);
					WriteAMF(pState,reader.readValue(),reader);
					lua_rawset(pState,-3);
					++size;
				}
				lua_pushnumber(pState,size);
				lua_setfield(pState,-2,"__size");
			}
			break;
		}
		case AMF::Object: {
			string objectType;
			if(reader.readObject(objectType)) {
				lua_newtable(pState);
				if(!objectType.empty()) {
					lua_pushlstring(pState,objectType.c_str(),objectType.size());
					lua_setfield(pState,-2,"__type");
				}
				string name;
				while((type=reader.readItem(name))!=AMF::End && type!=AMF::RawObjectContent) {
					WriteAMF(pState,type,reader);
					lua_setfield(pState,-2,name.c_str());
				}
				if(!objectType.empty()) {
					int top = lua_gettop(pState);
					// function
					SCRIPT_FUNCTION_BEGIN("onTypedObject")
						// type argument
						lua_pushlstring(pState,objectType.c_str(),objectType.size());
						// table argument
						lua_pushvalue(pState,top);
						SCRIPT_FUNCTION_CALL
					SCRIPT_FUNCTION_END
				}
				// After the "onTypedObject" to get before the "__readExternal" required to unserialize
				if(type==AMF::RawObjectContent) {
					WriteAMF(pState,type,reader);
					if(reader.readItem(name)!=AMF::End)
						ERROR("Something after a 'AMF::RawObjectContent' type");
				}
			}
			break;
		}
		case AMF::ByteArray: {
			UInt32 size;
			reader.readByteArray(size);
			lua_newtable(pState);
			lua_pushlstring(pState,(const char*)reader.reader.current(),size);
			lua_setfield(pState,-2,"__raw");
			reader.reader.next(size);
			break;
		}
		case AMF::RawObjectContent: {
			// Function
			lua_getfield(pState,-1,"__readExternal");
			if(lua_isfunction(pState,-1)) {
				// self
				lua_pushvalue(pState,-2);
				// reader
				SCRIPT_WRITE_OBJECT(AMFReader,LUAByteReader,reader)
				Service::StartVolatileObjectsRecording(pState);
				if(lua_pcall(pState,2,0,0)!=0)
					SCRIPT_ERROR("%s",Script::LastError(pState))
				Service::StopVolatileObjectsRecording(pState);
				break;
			}
			
			lua_getfield(pState,-2,"__type");
			SCRIPT_ERROR("Impossible to deserialize the type '%s' which implements IExternalized without a __readExternal method",lua_tostring(pState,-1))
			lua_pop(pState,2);
			break;
		}
		default:
			reader.reader.next(1);
			SCRIPT_ERROR("AMF %u type unknown",type);
			break;
	}
	SCRIPT_END
}

void Script::ReadAMF(lua_State* pState,AMFWriter& writer,UInt32 count) {
	map<UInt64,UInt32> references;
	ReadAMF(pState,writer,count,references);
}

void Script::ReadAMF(lua_State* pState,AMFWriter& writer,UInt32 count,map<UInt64,UInt32>& references) {
	SCRIPT_BEGIN(pState)
	int top = lua_gettop(pState);
	Int32 args = top-count;
	if(args<0) {
		SCRIPT_ERROR("Impossible to write %u missing AMF arguments",-args)
		args=0;
	}
	while(args++<top) {
		int type = lua_type(pState, args);
		switch (type) {
			case LUA_TTABLE: {

				// Repeat
				UInt64 reference = (UInt64)lua_topointer(pState,args);
				map<UInt64,UInt32>::iterator it = references.lower_bound(reference);
				if(it!=references.end() && it->first==reference) {
					if(writer.repeat(it->second))
						break;
				} else { 
					if(it!=references.begin())
						--it;
					it = references.insert( it,pair<UInt64,UInt32>(reference,0));
				}

				// ByteArray
				lua_getfield(pState,args,"__raw");
				if(lua_isstring(pState,-1)) {
					UInt32 size = lua_objlen(pState,-1);
					writer.writeByteArray(size).writeRaw(lua_tostring(pState,-1),size);
					it->second = writer.lastReference;
					lua_pop(pState,1);
					break;
				}
				lua_pop(pState,1);

				// Date
				lua_getfield(pState,args,"__time");
				if(lua_isnumber(pState,-1)) {
					Timestamp date((Timestamp::TimeVal)lua_tonumber(pState,-1)*1000);
					writer.writeDate(date);
					it->second = writer.lastReference;
					lua_pop(pState,1);
					break;
				}
				lua_pop(pState,1);


				// Dictionary
				UInt32 size=0;
				lua_getfield(pState,args,"__size");
				if(lua_isnumber(pState,-1)) {
					size = (UInt32)lua_tonumber(pState,-1);
	
					bool weakKeys = false;
					if(lua_getmetatable(pState,args)!=0) {
						lua_getfield(pState,-1,"__mode");
						if(lua_isstring(pState,-1) && strcmp(lua_tostring(pState,-1),"k")==0)
							weakKeys=true;
						lua_pop(pState,2);
					}
					
					// Array, write properties in first
					writer.beginDictionary(size,weakKeys);
					lua_pushnil(pState);  /* first key */
					while (lua_next(pState, args) != 0) {
						/* uses 'key' (at index -2) and 'value' (at index -1) */
						if(lua_type(pState,-2)!=LUA_TSTRING || strcmp(lua_tostring(pState,-2),"__size")!=0 )
							ReadAMF(pState,writer,2);
						/* removes 'value'; keeps 'key' for next iteration */
						lua_pop(pState, 1);
					}
					writer.endDictionary();

					it->second = writer.lastReference;
					lua_pop(pState,1);
					break;
				}
				lua_pop(pState,1);


				bool object=true;
				bool start=false;
				if(size==0)
					size = luaL_getn(pState,args);
				
				// Object or Array!
				lua_getfield(pState,args,"__type");
				const char* objectType = lua_isstring(pState,-1) ? lua_tostring(pState,-1) : NULL;
				if(objectType) {
					// Externalized?
					lua_getfield(pState,args,"__writeExternal");
					if(lua_isfunction(pState,-1)) {
						writer.beginExternalizableObject(objectType);
						// self argument
						lua_pushvalue(pState,args);
						// type argument
						SCRIPT_WRITE_OBJECT(AMFWriter,LUAByteWriter,writer)
						Service::StartVolatileObjectsRecording(pState);
						if(lua_pcall(pState,2,0,0)!=0)
							SCRIPT_ERROR("%s",Script::LastError(pState))
						Service::StopVolatileObjectsRecording(pState);
						writer.endExternalizableObject();
						it->second = writer.lastReference;
						lua_pop(pState,1);
						break;
					}
					lua_pop(pState,1);
					writer.beginObject(objectType);
					start=true;
				} else if(size>0) {
					// Array, write properties in first
					writer.beginObjectArray(size);
					object=false;
					lua_pushnil(pState);  /* first key */
					while (lua_next(pState, args) != 0) {
						/* uses 'key' (at index -2) and 'value' (at index -1) */
						int keyType = lua_type(pState,-2);
						const char* key = NULL;
						if(keyType==LUA_TSTRING && strcmp((key=lua_tostring(pState,-2)),"__type")!=0 ) {
							writer.writePropertyName(key);
							ReadAMF(pState,writer,1);
						}
						/* removes 'value'; keeps 'key' for next iteration */
						lua_pop(pState, 1);
					}
					writer.endObject();
					start=true;
				}
				lua_pop(pState,1);
				

				lua_pushnil(pState);  /* first key */
				while (lua_next(pState, args) != 0) {
					/* uses 'key' (at index -2) and 'value' (at index -1) */
					int keyType = lua_type(pState,-2);
					const char* key = NULL;
					if(keyType==LUA_TSTRING) {
						if(object && strcmp((key=lua_tostring(pState,-2)),"__type")!=0) {
							if(!start) {
								writer.beginObject();
								start=true;
							}
							writer.writePropertyName(key);
							ReadAMF(pState,writer,1);
						}
					} else if(keyType==LUA_TNUMBER) {
						if(object)
							SCRIPT_WARN("Impossible to encode this array element in an AMF object format")
						else
							ReadAMF(pState,writer,1);
					} else
						SCRIPT_WARN("Impossible to encode this table key of type %s in an AMF object format",lua_typename(pState,keyType))
					/* removes 'value'; keeps 'key' for next iteration */
					lua_pop(pState, 1);
				}
				if(start) {
					if(object)
						writer.endObject();
					else
						writer.endArray();
				} else {
					writer.beginArray(0);
					writer.endArray();
				}

				it->second = writer.lastReference;
				break;
			}
			case LUA_TSTRING: {
				string data(lua_tostring(pState,args),lua_objlen(pState,args));
				writer.write(data);
				break;
			}
			case LUA_TBOOLEAN:
				writer.writeBoolean(lua_toboolean(pState,args)==0 ? false : true);
				break;
			case LUA_TNUMBER: {
				double value = lua_tonumber(pState,args);
				if(value<=AMFWriter::AMF_MAX_INTEGER && ROUND(value) == value)
					writer.writeInteger((Int32)value);
				else
					writer.writeNumber(value);
				break;
			}
			default:
				SCRIPT_WARN("Impossible to encode the '%s' type in a AMF format",lua_typename(pState,type))
			case LUA_TNIL:
				writer.writeNull();
				break;
		}
	}
	SCRIPT_END
}

const char* Script::ToString(lua_State* pState,string& out) {
	int type = lua_type(pState, -1);
	string pointer;
	switch (lua_type(pState, -1)) {
		case LUA_TLIGHTUSERDATA:
			pointer = "userdata_";
			break;
		case LUA_TFUNCTION:
			pointer = "function_";
			break;
		case LUA_TUSERDATA:
			pointer = "userdata_";
			break;
		case LUA_TTHREAD:
			pointer = "thread_";
			break;
		case LUA_TTABLE: {
			pointer = "table_";
			break;
		}
		case LUA_TBOOLEAN: {
			out = lua_toboolean(pState,-1) ? "(true)" : "(false)";
			return out.c_str();
		}
		case LUA_TNIL: {
			out = "(null)";
			return out.c_str();
		}
	}
	if(pointer.empty())
		out = lua_tostring(pState,-1);
	else
		out = pointer + NumberFormatter::format(lua_topointer(pState,-1));
	return out.c_str();
}

bool Script::CheckType(lua_State *pState,const string& type) {
	if(lua_gettop(pState)==0 || lua_getmetatable(pState,-1)==0)
		return false;

	// __type is correct?
	lua_getfield(pState,-1,"__type");
	if(!lua_isstring(pState,-1) || (type!=lua_tostring(pState,-1))) {
		lua_pop(pState,2);
		return false;
	}
	lua_pop(pState,2);
	return true;
}

void Script::AddObjectDestructor(lua_State *pState,lua_CFunction destructor) {
	if(lua_istable(pState,-1)==0) {
		SCRIPT_BEGIN(pState)
			SCRIPT_ERROR("Add destructor impossible, bad object (not table)");
		SCRIPT_END
		return;
	}

	if(lua_getmetatable(pState,-1)==0) {
		SCRIPT_BEGIN(pState)
			SCRIPT_ERROR("Add destructor impossible, bad object (without metatable)");
		SCRIPT_END
		return;
	}

	// user data
	lua_newuserdata(pState,sizeof(void*));

	lua_pushvalue(pState,-2); // metatable
	lua_setmetatable(pState,-2); // metatable of user data

	lua_pushcfunction(pState,destructor);
	lua_setfield(pState,-3,"__gc"); // function in metatable

	lua_setfield(pState, -2, "__gcThis"); // userdata in metatable

	lua_pop(pState,1);
}
