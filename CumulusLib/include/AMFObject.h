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

#include "Cumulus.h"
#include "Poco/Util/MapConfiguration.h"

#define AMF_NUMBER			0x00
#define AMF_BOOLEAN			0x01
#define AMF_STRING			0x02
#define AMF_BEGIN_OBJECT	0x03
#define AMF_NULL			0x05
#define AMF_UNDEFINED		0x06
#define AMF_END_OBJECT		0x09
#define	AMF_AVMPLUS_OBJECT	0x11
#define	AMF_LONG_STRING		0x0C

namespace Cumulus {


class AMFObject : public Poco::Util::MapConfiguration {
public:
	AMFObject();
	virtual ~AMFObject();

	void setString(const std::string& key, const std::string& value);
	void setInt(const std::string& key, int value);
	void setDouble(const std::string& key, double value);
	void setBool(const std::string& key, bool value);

	bool has(const std::string& key) const;

};

inline bool AMFObject::has(const std::string& key) const {
	return hasProperty(key);
}


} // namespace Cumulus
