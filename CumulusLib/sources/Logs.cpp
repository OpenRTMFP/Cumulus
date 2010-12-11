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

#include "Logs.h"
#include "Poco/File.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

Logger* Logs::s_pLogger(NULL);
string	Logs::s_file;
bool	Logs::s_dump(false);
UInt8	Logs::s_level(Logger::PRIO_INFO); // default log level

Logs::Logs() {
}

Logs::~Logs() {
}

void Logs::Dump(bool activate,const string& file) {
	s_file = file;
	s_dump=activate;
	if(s_dump && !s_file.empty()) {
		File dumpFile(s_file);
		if(dumpFile.exists())
			dumpFile.remove();
	}
}

void Logs::SetLevel(UInt8 level) {
	s_level = level;
}



} // namespace Cumulus