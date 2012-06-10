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

#include "Address.h"
#include "Poco/StringTokenizer.h"
#include "Poco/String.h"
#include <stdlib.h>


using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Address::Address() : host(4),port(0) {
}

Address::Address(const string& host,UInt16 port) : host(4),port(port) {
	buildHost(host);
}

Address::Address(const string& address) : host(4),port(0) {
	// port
	size_t pos = address.find_last_of(':');
	if(pos!=string::npos)
		((UInt16&)port) = atoi(&address.c_str()[pos+1]);
	else
		pos = address.size();

	string host(address,0,pos);
	buildHost(host);
}

Address::~Address() {
	
}

void Address::buildHost(const std::string& host) {
	// host
	if(host.find_first_of(':')==string::npos) {
		// IPv4
		StringTokenizer split(host,".",StringTokenizer::TOK_TRIM);
		StringTokenizer::Iterator it;
		int i=0;
		for(it=split.begin();it!=split.end();++it) {
			if(i>=this->host.size())
				return;
			((vector<UInt8>&)this->host)[i++] = atoi(it->c_str());
		}
	} else {
		// IPv6
		((vector<UInt8>&)this->host).resize(16);
		size_t first = host.find_first_of('[');
		if(first == string::npos)
			first = 0;
		else
			++first;
		size_t size = host.find_first_of(']');
		if(size == string::npos)
			size = host.size()-first;
		else
			size -= first;
		StringTokenizer split(string(host,first,size),":",StringTokenizer::TOK_TRIM);
		StringTokenizer::Iterator it;
		int i=0;
		for(it=split.begin();it!=split.end();++it) {
			string temp(*it);
			int delta = 4-it->size();
			if(delta>0)
				temp.insert(0,string(delta,'0'));

			int c = 0;
			for(int j=0;j<4;++j) {
				int n = temp[j];
				if (n >= '0' && n <= '9')
					c |= n - '0';
				else if (n >= 'A' && n <= 'F')
					c |= n - 'A' + 10;
				else if (n >= 'a' && n <= 'f')
					c |= n - 'a' + 10;
				if(j%2==0)
					c <<= 4;
				else {
					if(i>=this->host.size())
						return;
					((vector<UInt8>&)this->host)[i++] = c;
					c = 0;
				}
			}
		}
	}
}

Address& Address::operator=(const Address& other) {
	((UInt16&)port) = other.port;
	((vector<UInt8>&)host) = other.host;
	return *this;
}

bool Address::operator==(const SocketAddress& address) const {
	if(port != address.port())
		return false;
	IPAddress tmp = address.host();
	const UInt8* bytes = reinterpret_cast<const UInt8*>(tmp.addr());
	if(tmp.family() == IPAddress::IPv6) {
		if(host.size()!=16)
			return false;
	} else {
		if(host.size()!=4)
			return false;
	}
	for(int i=0;i<host.size();++i) {
		if(bytes[i]!=host[i])
			return false;
	}
	return true;
}




} // namespace Cumulus
