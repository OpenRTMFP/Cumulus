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
*/

#pragma once

#include "Cumulus.h"
#include "PacketReader.h"
#include "PacketWriter.h"
#include <map>


namespace Cumulus {

class Util {
public:
	Util();
	~Util();

	static std::string FormatHex(const Poco::UInt8* data,unsigned size);
	static void Dump(const Poco::UInt8* data,int size,const std::string& fileName="");
	static void Dump(PacketReader& packet,const std::string& fileName="");
	static void Dump(PacketWriter& packet,const std::string& fileName="");
	static void Dump(PacketWriter& packet,Poco::UInt16 offset,const std::string& fileName="");
	static void UnpackUrl(const std::string& url,std::string& path,std::map<std::string,std::string>& parameters);
};

} // namespace Cumulus
