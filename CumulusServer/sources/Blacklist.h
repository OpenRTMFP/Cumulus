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
#include "Invoker.h"

class Blacklist : public FileWatcher {
public:
	Blacklist(const std::string& path,Cumulus::Invoker& invoker);
	virtual ~Blacklist();

private:
	void load();
	void clear();

	Cumulus::Invoker&	_invoker;
};

inline void Blacklist::clear() {
	_invoker.clearBannedList();
}
