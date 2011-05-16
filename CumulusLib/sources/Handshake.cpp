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

#include "Handshake.h"
#include "Logs.h"
#include "Util.h"
#include "Poco/RandomStream.h"
#include <openssl/evp.h>
#include "string.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Handshake::Handshake(Gateway& gateway,DatagramSocket& socket,ServerHandler& serverHandler) : Session(0,0,Peer(),RTMFP_SYMETRIC_KEY,RTMFP_SYMETRIC_KEY,socket,serverHandler),
	_gateway(gateway) {
	
	memcpy(_certificat,"\x01\x0A\x41\x0E",4);
	RandomInputStream().read((char*)&_certificat[4],64);
	memcpy(&_certificat[68],"\x02\x15\x02\x02\x15\x05\x02\x15\x0E",9);

	// Display far id flash side
	EVP_Digest(_certificat,sizeof(_certificat),(unsigned char *)serverHandler.id,NULL,EVP_sha256(),NULL);

	INFO("Id of this cumulus server : %s",Util::FormatHex(serverHandler.id,ID_SIZE).c_str());
}


Handshake::~Handshake() {
	clear();
}

void Handshake::manage() {
	// delete obsolete cookie
	map<string,Cookie*>::iterator it=_cookies.begin();
	while(it!=_cookies.end()) {
		if(it->second->obsolete()) {
			delete it->second;
			_cookies.erase(it++);
		} else
			++it;
	}
}

void Handshake::commitCookie(const Session& session) {
	(bool&)session.checked = true;
	map<string,Cookie*>::iterator it;
	for(it=_cookies.begin();it!=_cookies.end();++it) {
		if(it->second->id==session.id()) {
			delete it->second;
			_cookies.erase(it);
			return;
		}
	}
	WARN("Cookie of the session '%d' not found",session.id());
}

void Handshake::clear() {
	// delete cookies
	map<string,Cookie*>::const_iterator it;
	for(it=_cookies.begin();it!=_cookies.end();++it)
		delete it->second;
	_cookies.clear();
}

void Handshake::createCookie(PacketWriter& writer,Cookie* pCookie) {
	// New Cookie
	char cookie[65];
	RandomInputStream ris;
	ris.read(cookie,64);
	cookie[64]='\0';
	writer.write8(64);
	writer.writeRaw(cookie,64);
	_cookies[cookie] = pCookie;
}

void Handshake::packetHandler(PacketReader& packet) {

	UInt8 marker = packet.read8();
	if(marker!=0x0b) {
		ERROR("Marker handshake wronk : must be '0B' and not '%02x'",marker);
		return;
	}
	
	UInt16 time = packet.read16();
	UInt8 id = packet.read8();
	packet.shrink(packet.read16()); // length

	PacketWriter& packetOut(writer());
	UInt8 idResponse=0;
	{
		PacketWriter response(packetOut,3);
		idResponse = handshakeHandler(id,packet,response);
		if(idResponse==0)
			return;
	}

	packetOut << (UInt8)idResponse;
	packetOut << (UInt16)(packetOut.length()-packetOut.position()-2);

	flush(SYMETRIC_ENCODING | WITHOUT_ECHO_TIME);
	// reset farid to 0!
	_farId=0;
}


UInt8 Handshake::handshakeHandler(UInt8 id,PacketReader& request,PacketWriter& response) {

	switch(id){
		case 0x30: {
			
			request.read8(); // passer un caractere (boite dans boite)
			UInt8 epdLen = request.read8()-1;

			
			UInt8 type = request.read8();

			string epd;
			request.readRaw(epdLen,epd);

			string tag;
			request.readRaw(16,tag);
			response.writeString8(tag);

			
			if(type == 0x0f)
				return _gateway.p2pHandshake(tag,response,peer().address,(const UInt8*)epd.c_str());

			if(type == 0x0a){
				/// Handshake
	
				// New Cookie
				createCookie(response,new Cookie(epd));
				 
				// instance id (certificat in the middle)
				response.writeRaw(_certificat,sizeof(_certificat));
				
				return 0x70;
			} else {
				ERROR("Unkown handshake first way with '%02x' type",type);
			}
			break;
		}
		case 0x38: {
			_farId = request.read32();
			string idCookie;
			request.readString(idCookie);

			map<string,Cookie*>::iterator itCookie = _cookies.find(idCookie.c_str());
			if(itCookie==_cookies.end()) {
				ERROR("Handshake cookie '%s' unknown",idCookie.c_str());
				return 0;
			}
			Cookie& cookie(*itCookie->second);

			if(cookie.id==0) {

				UInt32 size = request.read7BitValue();
				// peerId = SHA256(farPubKey)
				EVP_Digest(request.current(),size,(UInt8*)peer().id,NULL,EVP_sha256(),NULL);

				request.next(size-KEY_SIZE);
				UInt8 publicKey[KEY_SIZE];
				request.readRaw(publicKey,KEY_SIZE);

				size = request.read7BitValue();

				UInt8 decryptKey[KEY_SIZE];
				UInt8 encryptKey[KEY_SIZE];
				cookie.computeKeys(publicKey,request.current(),size,decryptKey,encryptKey);

				// Fill peer infos
				Util::UnpackUrl(cookie.queryUrl,(string&)peer().path,(map<string,string>&)peer().parameters);

				// RESPONSE
				(UInt32&)cookie.id = _gateway.createSession(_farId,peer(),decryptKey,encryptKey,cookie);
				cookie.write(response);
			} else
				cookie.write(response);

			return 0x78;
		}
		default:
			ERROR("Unkown handshake packet id '%02x'",id);
	}

	return 0;
}




} // namespace Cumulus
