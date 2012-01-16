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


class Service : public FileWatcher {
public:
	Service(lua_State* pState,const std::string& path);
	virtual ~Service();

	static void InitGlobalTable(lua_State *pState);
	static void StartVolatileObjectsRecording(lua_State* pState);
	static void StopVolatileObjectsRecording(lua_State* pState);

	Service*	get(const std::string& path);

	void		refresh();
	lua_State*	open();

	Poco::UInt32 count;
private:
	
	bool		open(bool create);
	void		watch();
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

	static bool						_VolatileObjectsRecording;
	static Poco::Thread*			_PVolatileObjectsThreadRecording;
};

inline void Service::InitGlobalTable(lua_State* pState) {
	InitGlobalTable(pState,false);
}

inline void Service::watch() {
	FileWatcher::watch();
}

inline void Service::refresh() {
	FileWatcher::watch();
}
