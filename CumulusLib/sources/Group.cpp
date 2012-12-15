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

#include "Group.h"
#include "string.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

UInt32 Group::Distance(Iterator& it0,Iterator& it1) {
	return distance(it0._it,it1._it);
}

void Group::Advance(Iterator& it,UInt32 count) {
	advance(it._it,count);
}

Group::Group(const UInt8* id) {
	memcpy((UInt8*)this->id,id,ID_SIZE);
}

Group::~Group() {

}


} // namespace Cumulus
