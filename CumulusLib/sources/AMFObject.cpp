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

#include "AMFObject.h"

using namespace std;

namespace Cumulus {

AMFObject::AMFObject() {
	
}


AMFObject::~AMFObject() {
}

void AMFObject::setString(const string& key, const string& value) {
	MapConfiguration::setInt(key+".type",value.empty() ? AMF_UNDEFINED : AMF_STRING);
	MapConfiguration::setString(key,value);
}

void AMFObject::setInt(const string& key, int value) {
	MapConfiguration::setInt(key+".type",AMF_NUMBER);
	MapConfiguration::setInt(key,value);
}

void AMFObject::setDouble(const string& key, double value) {
	MapConfiguration::setInt(key+".type",AMF_NUMBER);
	MapConfiguration::setDouble(key,value);
}

void AMFObject::setBool(const string& key, bool value) {
	MapConfiguration::setInt(key+".type",AMF_BOOLEAN);
	MapConfiguration::setBool(key,value);
}




} // namespace Cumulus
