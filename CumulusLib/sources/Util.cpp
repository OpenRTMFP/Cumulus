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
#include "Poco/FileStream.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

Util::Util() {
}

Util::~Util() {
}

void Util::Dump(PacketReader& packet,const string& fileName) {
	Dump(packet.current(),packet.available(),fileName);
}
void Util::Dump(PacketWriter& packet,const string& fileName) {
	Dump(packet.begin(),packet.length(),fileName);
}
void Util::Dump(PacketWriter& packet,UInt16 offset,const string& fileName) {
	Dump(packet.begin()+offset,packet.length()-offset,fileName);
}


void Util::Dump(const UInt8* data,int size,const string& fileName) {
	int i = 0;
	int c = 0;
	unsigned char b;
	while (i<size) {
		c = 0;
		while ( (c < 16) && (i+c < size) ) {
			b = data[i+c];
			printf("%X%X ", b/16, b & 0x0f );
			++c;
		}
		while (c++ < 16)
			printf("   ");
		c = 0;
		while ( (c < 16) && (i+c < size) ) {
			b = data[i+c];
			if (b > 31)
				printf("%c", (char)b );
			else
				printf(".");
			++c;
		}
		i += 16;
		printf("\n");
	}
	printf("\n");
	if(!fileName.empty()) {
		FileOutputStream fos(fileName,std::ios::app | std::ios::out);
		fos.write((char*)data,size);
		c -= 16;
		// add 0xFF end-padding
		while(c<16) {
			fos.put((UInt8)0xFF);
			++c;
		}
		fos.close();
	}
}


} // namespace Cumulus
