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
#include "Poco/HexBinaryEncoder.h"
#include <openssl/evp.h>

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Handshake::Handshake(Gateway& gateway,DatagramSocket& socket,ServerData& data) : Session(0,0,Peer(),"",RTMFP_SYMETRIC_KEY,RTMFP_SYMETRIC_KEY,socket,data),
	_gateway(gateway),_signature("\x03\x1a\x00\x00\x02\x1e\x00\x81\x02\x0d\x02",11) {
	
	memcpy(_certificat,"\x01\x0A\x41\x0E",4);
	RandomInputStream().read((char*)&_certificat[4],64);
	memcpy(&_certificat[68],"\x02\x15\x02\x02\x15\x05\x02\x15\x0E",9);

	// Display far id flash side
	Poco::UInt8 flashFarId[32];
	EVP_Digest(_certificat,sizeof(_certificat),flashFarId,NULL,EVP_sha256(),NULL);

	char printFarId[65];
	MemoryOutputStream mos(printFarId,65);
	HexBinaryEncoder(mos).write((char*)flashFarId,32);
	mos.put('\0');
	INFO("Flash far id of this cumulus server : %s",printFarId);
}


Handshake::~Handshake() {
	// delete cookies // TODO quand effacer les vieux cookies jamais utilisé?
	map<string,Cookie*>::const_iterator it;
	for(it=_cookies.begin();it!=_cookies.end();++it)
		delete it->second;
	_cookies.clear();
	
}

void Handshake::packetHandler(PacketReader& packet) {

	UInt8 marker = packet.next8();
	if(marker!=0x0b) {
		ERROR("Marker handshake wronk : must be '0B' and not '%02x'",marker);
		return;
	}
	
	UInt16 time = packet.next16();
	UInt8 id = packet.next8();
	UInt16 length = packet.next16();
	packet.reset(packet.position(),packet.position()+length);

	PacketWriter packetOut(9); // 9 for future marker, timestamp, crc 2 bytes and id session 4 bytes
	UInt8 idResponse=0;
	{
		PacketWriter response = packetOut;
		response.skip(3);
		idResponse = handshakeHandler(id,packet,response);
		if(idResponse==0)
			return;
	}

	packetOut << (UInt8)idResponse;
	packetOut << (UInt16)(packetOut.size()-packetOut.position()-2);

	send(marker,packetOut,true);
	// reset farid to 0!
	_farId=0;
}


// TODO make perhaps a FlowHandshake
UInt8 Handshake::handshakeHandler(UInt8 id,PacketReader& request,PacketWriter& response) {

	switch(id){
		case 0x30: {
			
			request.next8(); // passer un caractere (boite dans boite)
			UInt8 epdLen = request.next8()-1;

			
			UInt8 type = request.next8();

			string epd;
			request.readRaw(epdLen,epd);

			string tag;
			request.readRaw(16,tag);
			response.writeString8(tag);

			// UDP hole punching

			if(type == 0x0f) // mode man in the middle
				return _gateway.p2pHandshake(tag,response,peer(),(const UInt8*)epd.c_str());

			if(type == 0x0a){ // TODO vérifier ça!
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
			_farId = request.next32();
			string cookie;
			request.readRaw(request.next8(),cookie);

			map<string,Cookie*>::iterator itCookie = _cookies.find(cookie.c_str());
			if(itCookie==_cookies.end()) {
				ERROR("Handshake cookie '0x38' unknown");
				return 0;
			}

			request.next8(); // why 0x81?

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
			RTMFP::ComputeAsymetricKeys(sharedSecret,requestKey,responseKey,pubKey,_signature,farCertificat);

			// RESPONSE
			response << _gateway.createSession(_farId,peer(),itCookie->second->url,requestKey,responseKey);
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
