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

#include "AMFSimpleObject.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

AMFSimpleObject::AMFSimpleObject() {
	
}


AMFSimpleObject::~AMFSimpleObject() {
}

void AMFSimpleObject::setString(const string& key, const string& value) {
	MapConfiguration::setInt(key+".type",AMF::String);
	MapConfiguration::setString(key,value);
}

void AMFSimpleObject::setInteger(const string& key, int value) {
	MapConfiguration::setInt(key+".type",AMF::Integer);
	MapConfiguration::setInt(key,value);
}

void AMFSimpleObject::setNumber(const string& key, double value) {
	MapConfiguration::setInt(key+".type",AMF::Number);
	MapConfiguration::setDouble(key,value);
}

void AMFSimpleObject::setBoolean(const string& key, bool value) {
	MapConfiguration::setInt(key+".type",AMF::Boolean);
	MapConfiguration::setBool(key,value);
}

void AMFSimpleObject::setDate(const string& key, Timestamp& date) {
	MapConfiguration::setInt(key+".type",AMF::Date);
	MapConfiguration::setDouble(key,(double)(date.epochMicroseconds()/1000));
}

void AMFSimpleObject::setNull(const string& key) {
	MapConfiguration::setInt(key+".type",AMF::Null);
	MapConfiguration::setInt(key,0);
}


} // namespace Cumulus
