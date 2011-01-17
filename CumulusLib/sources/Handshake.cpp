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
#include "Middle.h"
#include "Poco/RandomStream.h"
#include <openssl/evp.h>
#include "string.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Handshake::Handshake(Gateway& gateway,DatagramSocket& socket,ServerData& data) : Session(0,0,Peer(),RTMFP_SYMETRIC_KEY,RTMFP_SYMETRIC_KEY,socket,data),
	_gateway(gateway),_signature("\x03\x1a\x00\x00\x02\x1e\x00\x81\x02\x0d\x02",11) {
	
	memcpy(_certificat,"\x01\x0A\x41\x0E",4);
	RandomInputStream().read((char*)&_certificat[4],64);
	memcpy(&_certificat[68],"\x02\x15\x02\x02\x15\x05\x02\x15\x0E",9);

	// Display far id flash side
	Poco::UInt8 flashFarId[32];
	EVP_Digest(_certificat,sizeof(_certificat),flashFarId,NULL,EVP_sha256(),NULL);

	INFO("Flash far id of this cumulus server : %s",Util::FormatHex(flashFarId,32).c_str());
}


Handshake::~Handshake() {
	clear();
}

void Handshake::clear() {
	// delete cookies // TODO quand effacer les vieux cookies jamais utilisé?
	map<string,Cookie*>::const_iterator it;
	for(it=_cookies.begin();it!=_cookies.end();++it)
		delete it->second;
	_cookies.clear();
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

	PacketWriter& packetOut = writer();
	UInt8 idResponse=0;
	{
		PacketWriter response(packetOut,3);
		idResponse = handshakeHandler(id,packet,response);
		if(idResponse==0)
			return;
	}

	packetOut << (UInt8)idResponse;
	packetOut << (UInt16)(packetOut.length()-packetOut.position()-2);

	send(true);
	// reset farid to 0!
	_farId=0;
}


// TODO make perhaps a FlowHandshake
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

			// UDP hole punching

			if(type == 0x0f)
				return _gateway.p2pHandshake(tag,response,peer().address(),(const UInt8*)epd.c_str());

			if(type == 0x0a){
				/// Handshake
	
				// RESPONSE 38

				// New Cookie
				char cookie[65];
				RandomInputStream ris;
				ris.read(cookie,64);
				cookie[64]='\0';
				response.write8(64);
				response.writeRaw(cookie,64);
				_cookies[cookie] = new Cookie(epd);
				 
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
			string cookie;
			request.readRaw(request.read8(),cookie);

			map<string,Cookie*>::iterator itCookie = _cookies.find(cookie.c_str());
			if(itCookie==_cookies.end()) {
				ERROR("Handshake cookie '%s' unknown",cookie.c_str());
				return 0;
			}

			request.read8(); // why 0x81?

			// signature
			string farSignature;
			request.readString8(farSignature); // 81 02 1D 02 stable

			// farPubKeyPart
			UInt8 farPubKeyPart[128];
			request.readRaw((char*)farPubKeyPart,sizeof(farPubKeyPart));

			string farCertificat;
			request.readString8(farCertificat);
			
			// peerId = SHA256(farSignature+farPubKey)
			string temp(farSignature);
			temp.append((char*)farPubKeyPart,sizeof(farPubKeyPart));
			EVP_Digest(temp.c_str(),temp.size(),(UInt8*)peer().id,NULL,EVP_sha256(),NULL);
			
			// Compute Diffie-Hellman secret
			UInt8 pubKey[128];
			UInt8 sharedSecret[128];
			RTMFP::EndDiffieHellman(RTMFP::BeginDiffieHellman(pubKey),farPubKeyPart,sharedSecret);

			// Compute Keys
			UInt8 requestKey[AES_KEY_SIZE];
			UInt8 responseKey[AES_KEY_SIZE];
			RTMFP::ComputeAsymetricKeys(sharedSecret,pubKey,_signature,farCertificat,requestKey,responseKey);

			// RESPONSE
			Util::UnpackUrl(itCookie->second->queryUrl,(string&)peer().path,(map<string,string>&)peer().parameters);
			response << _gateway.createSession(_farId,peer(),requestKey,responseKey);
			response.write8(0x81);
			response.writeString8(_signature);
			response.writeRaw((char*)pubKey,sizeof(pubKey));
			response.write8(0x58);

			// remove cookie
			_cookies.erase(itCookie);

			// db_add_public(s); // TODO

			return 0x78;
		}
		default:
			ERROR("Unkown handshake packet id '%02x'",id);
	}

	return 0;
}




} // namespace Cumulus
