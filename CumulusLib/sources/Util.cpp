/* 
	Copyright 2010 cumulus.dev@gmail.com
 
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

void Util::Dump(PacketReader& packet,const string& fileName,bool justFile) {
	Dump(packet.current(),packet.available(),fileName,justFile);
}
void Util::Dump(PacketWriter& packet,const string& fileName,bool justFile) {
	Dump(packet.begin(),packet.size(),fileName,justFile);
}
void Util::Dump(PacketWriter& packet,UInt16 offset,const string& fileName,bool justFile) {
	Dump(packet.begin()+offset,packet.size()-offset,fileName,justFile);
}


void Util::Dump(const UInt8* sz,int size,const string& fileName,bool justFile) {
	int i = 0;
	int c = 0;
	unsigned char b;
	while (i<size) {
		c = 0;
		while ( (c < 16) && (i+c < size) ) {
			b = sz[i+c];
			if(!justFile)
				printf("%X%X ", b/16, b & 0x0f );
			c++;
		}
		while (c++ < 16) {
			if(!justFile)
				printf("   ");
		}
		c = 0;
		while ( (c < 16) && (i+c < size) ) {
			b = sz[i+c];
			if(!justFile) {
				if (b > 31)
					printf("%c", (char)b );
				else
					printf(".");
			}
			c++;
		}
		i += 16;
		if(!justFile)
			printf("\n");
	}
	if(!justFile)
		printf("\n");
	if(!fileName.empty()) {
		FileOutputStream fos(fileName,std::ios::app | std::ios::out);
		fos.write((char*)sz,size);
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
