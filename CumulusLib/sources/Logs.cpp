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
#include "Util.h"
#include "string.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

Logger*			Logs::_PLogger(NULL);
Logs::DumpMode	Logs::_DumpMode(NOTHING);
UInt8			Logs::_Level(Logger::PRIO_INFO); // default log level

Logs::Logs() {
}

Logs::~Logs() {
}


void Logs::Dump(const UInt8* data,UInt32 size,const char* header,bool middle) {
	UInt8 type = middle ? MIDDLE : EXTERNAL;
	if(GetLogger() && _DumpMode&type) {
		vector<UInt8> out;
		Util::Dump(data,size,out,header);
		if(out.size()>0)
			GetLogger()->dumpHandler((const char*)&out[0],out.size());
	}
}



} // namespace Cumulus
