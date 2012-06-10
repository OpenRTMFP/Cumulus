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

#include "FileWatcher.h"
#include "Script.h"
#include "Poco/StringTokenizer.h"

class Service;
class ServiceRegistry {
public:
	virtual void addFunction(Service& service,const std::string& name){}
	virtual void clear(Service& service){}
};


class Service : public FileWatcher {
public:
	Service(lua_State* pState,const std::string& path,ServiceRegistry& registry);
	virtual ~Service();

	static void InitGlobalTable(lua_State *pState);
	static void StartVolatileObjectsRecording(lua_State* pState);
	static void StopVolatileObjectsRecording(lua_State* pState);

	Service*	get(const std::string& path);

	bool		refresh();
	lua_State*	open();

	Poco::UInt32		count;
	const std::string	lastError;
private:
	
	bool		open(bool create);
	void		load();
	void		clear();

	static void	InitGlobalTable(lua_State* pState,bool pushMetatable);
	static int	Index(lua_State* pState);
	static int  NewIndex(lua_State* pState);

	bool					_running;
	lua_State*				_pState;
	bool					_deleting;
	Poco::StringTokenizer	_packages;
	std::string				_path;

	std::map<std::string,Service*>	_services;
	ServiceRegistry&				_registry;

	static bool						_VolatileObjectsRecording;
	static Poco::Thread*			_PVolatileObjectsThreadRecording;
};

inline void Service::InitGlobalTable(lua_State* pState) {
	InitGlobalTable(pState,false);
}

inline bool Service::refresh() {
	return FileWatcher::watch();
}
