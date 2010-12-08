/* 
	Copyright 2010 cumulus.dev@gmail.com
 
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

#include "BLOB.h"

using namespace Poco;
using namespace Poco::Data;

namespace Cumulus {

BLOB::BLOB() {

}

BLOB::BLOB(const Poco::UInt8* content, std::size_t size) : Poco::Data::BLOB((char*)content,size) {

}

BLOB::BLOB(const std::string& content) : Poco::Data::BLOB(content) {
	
}

BLOB::~BLOB() {

}


} // namespace Cumulus