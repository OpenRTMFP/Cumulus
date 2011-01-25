/* 
	Copyright 2010 OpenRTMFP
 
	This file is a part of Cumulus.
 
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

#include "Util.h"
#include "Poco/URI.h"
#include "Poco/FileStream.h"
#include "Poco/HexBinaryEncoder.h"
#include <sstream>

using namespace std;
using namespace Poco;

namespace Cumulus {

Util::Util() {
}

Util::~Util() {
}

string Util::FormatHex(const UInt8* data,unsigned size) {
	ostringstream oss;
	HexBinaryEncoder(oss).write((char*)data,size);
	return oss.str();
	//WARN("Unknown session for a peerId of '%s'",printPeerId);
}


void Util::UnpackUrl(const string& url,string& path,map<string,string>& parameters) {
	URI uri(url);
	path.assign(uri.getPath());
	string query = uri.getRawQuery();

	istringstream istr(query);
	static const int eof = std::char_traits<char>::eof();

	int ch = istr.get();
	while (ch != eof)
	{
		string name;
		string value;
		while (ch != eof && ch != '=' && ch != '&')
		{
			if (ch == '+') ch = ' ';
			name += (char) ch;
			ch = istr.get();
		}
		if (ch == '=')
		{
			ch = istr.get();
			while (ch != eof && ch != '&')
			{
				if (ch == '+') ch = ' ';
				value += (char) ch;
				ch = istr.get();
			}
		}
		string decodedName;
		string decodedValue;
		URI::decode(name, decodedName);
		URI::decode(value, decodedValue);
		parameters[decodedName] = decodedValue;
		if (ch == '&') ch = istr.get();
	}
}



} // namespace Cumulus
