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
#include "Logs.h"
#include "Poco/URI.h"
#include "Poco/FileStream.h"
#include "Poco/HexBinaryEncoder.h"
#include "string.h"
#include <sstream>
#include "math.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;
using namespace Poco::Util;

namespace Cumulus {

string Util::NullString;

Util::Util() {
}

Util::~Util() {
}

bool Util::SameAddress(const SocketAddress& address1,const SocketAddress& address2) {
	return memcmp(address1.addr(),address2.addr(),address1.length())==0 && address1.port() == address2.port();
}


string Util::FormatHex(const UInt8* data,UInt32 size) {
	ostringstream oss;
	HexBinaryEncoder(oss).write((char*)data,size);
	return oss.str();
}

UInt8 Util::Get7BitValueSize(UInt32 value) {
	if(value>=0x200000)
		return 4;
	if(value>=0x4000)
		return 3;
	if(value>=0x80)
		return 2;
	return 1;
}

void Util::UnpackUrl(const string& url,string& path,AbstractConfiguration& parameters) {
	try {
		URI uri(url);
		path = uri.getPath();
		UnpackQuery(uri.getRawQuery(),parameters);
	} catch(Exception& ex) {
		ERROR("Unpack url %s impossible : %s",url.c_str(),ex.displayText().c_str());
	}
}

void Util::UnpackQuery(const string& query,AbstractConfiguration& parameters) {
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
		parameters.setString(decodedName,decodedValue);
		if (ch == '&') ch = istr.get();
	}
}


void Util::Dump(const UInt8* in,UInt32 size,vector<UInt8>& out,const char* header) {
	UInt32 len = 0;
	UInt32 i = 0;
	UInt32 c = 0;
	UInt8 b;
	out.resize((UInt32)ceil((double)size/16)*67 + (header ? (strlen(header)+2) : 0));

	if(header) {
		out[len++] = '\t';
		c = strlen(header);
		memcpy(&out[len],header,c);
		len += c;
		out[len++] = '\n';
	}

	while (i<size) {
		c = 0;
		out[len++] = '\t';
		while ( (c < 16) && (i+c < size) ) {
			b = in[i+c];
			sprintf((char*)&out[len],"%X%X ",b>>4, b & 0x0f );
			len += 3;
			++c;
		}
		while (c++ < 16) {
			strcpy((char*)&out[len],"   ");
			len += 3;
		}
		out[len++] = ' ';
		c = 0;
		while ( (c < 16) && (i+c < size) ) {
			b = in[i+c];
			if (b > 31)
				out[len++] = b;
			else
				out[len++] = '.';
			++c;
		}
		while (c++ < 16)
			out[len++] = ' ';
		i += 16;
		out[len++] = '\n';
	}
}



} // namespace Cumulus
