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

#include "Client.h"
#include "string.h"

using namespace Poco;

namespace Cumulus {

Client::Client():id() {
	
}

Client::~Client() {
}

bool Client::operator==(const Client& other) const {
	return memcmp(id,other.id,32)==0;
}
bool Client::operator==(const UInt8* id) const {
	return memcmp(this->id,id,32)==0;
}

bool Client::operator!=(const Client& other) const {
	return memcmp(id,other.id,32)!=0;
}
bool Client::operator!=(const UInt8* id) const {
	return memcmp(this->id,id,32)!=0;
}



} // namespace Cumulus
