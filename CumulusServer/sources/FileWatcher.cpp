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

#include "FileWatcher.h"

using namespace std;
using namespace Poco;


FileWatcher::FileWatcher(const string& path) : _file(path),_lastModified(0),path(path) {

}


FileWatcher::~FileWatcher() {

}

bool FileWatcher::watch() {
	bool result=false;
	Timestamp lastModified=0;
	if(_lastModified>0) {
		if(!_file.exists() || !_file.isFile() || (lastModified=_file.getLastModified())>_lastModified) {
			_lastModified=lastModified;
			clear();
			if(_lastModified>0) {
				load();
				result=true;
			}
		}
	} else if(_file.exists() && _file.isFile()) {
		_lastModified = _file.getLastModified();
		load();
		result=true;
	}
	return result;
}
