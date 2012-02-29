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
#include "Poco/NullStream.h"
#include "Poco/Net/SocketAddress.h"
#include <vector>
#include <map>


namespace Cumulus {

class Util {
public:
	Util();
	~Util();

	static bool		   SameAddress(const Poco::Net::SocketAddress& address1,const Poco::Net::SocketAddress& address2);
	static std::string FormatHex(const Poco::UInt8* data,Poco::UInt32 size);
	static Poco::UInt8 Get7BitValueSize(Poco::UInt32 value);
	static Poco::UInt8 Get7BitValueSize(Poco::UInt64 value);

	static void Dump(const Poco::UInt8* in, Poco::UInt32 size,std::vector<Poco::UInt8>& out,const char* header=NULL);

	static void UnpackUrl(const std::string& url,std::string& path,std::map<std::string,std::string>& properties);
	static void UnpackQuery(const std::string& query,std::map<std::string,std::string>& properties);

	static std::string				NullString;
	static Poco::NullInputStream	NullInputStream;
	static Poco::NullOutputStream	NullOutputStream;
};

inline Poco::UInt8 Util::Get7BitValueSize(Poco::UInt32 value) {
	return Get7BitValueSize((Poco::UInt64)value);
}

} // namespace Cumulus
