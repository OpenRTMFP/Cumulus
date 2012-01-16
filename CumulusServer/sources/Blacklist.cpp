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

#include "Blacklist.h"
#include "Poco/String.h"
#include <fstream>

using namespace std;
using namespace Poco;
using namespace Poco::Net;
using namespace Cumulus;


Blacklist::Blacklist(const string& path,Invoker& invoker) : FileWatcher(path),_invoker(invoker) {
	watch();
}


Blacklist::~Blacklist() {
	clear();
}

void Blacklist::clear() {
	_invoker.clearBannedList();
}


void Blacklist::load() {

	/// Read auth file
	ifstream  istr(path.c_str());
	string line;
	while(getline(istr,line)) {
		trimInPlace(line);
		// remove comments
		size_t pos = line.find('#');
		if(pos!=string::npos) {
			line = line.substr(0,pos);
			trimInPlace(line);
		}
		
		if(!line.empty()) {
			try {
				_invoker.addBanned(IPAddress(line));
			} catch(Exception& ex) {
				ERROR("Incomprehensible blacklist entry, %s",ex.displayText().c_str());
			}
		}
	}
	
}
