/* 
	Copyright 2010 cumulus.dev@gmail.com
 
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

Handshake::Handshake(DatagramSocket& socket,const string& listenCirrusUrl) : Session(0,0,"",RTMFP_SYMETRIC_KEY,RTMFP_SYMETRIC_KEY,socket),
	_pNewSession(NULL),nextSessionId(0),_listenCirrusUrl(listenCirrusUrl),_signature("\x03\x1a\x00\x00\x02\x1e\x00\x81\x02\x0d\x02",11),_pOneMiddle(NULL) {
	
	memcpy(_certificat,"\x01\x0A\x41\x0E",4);
	RandomInputStream().read(&_certificat[4],64);
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

Session* Handshake::getNewSession() {
	Session* pSession = _pNewSession;
	_pNewSession = NULL;
	return pSession;
}

void Handshake::packetHandler(PacketReader& packet,SocketAddress& sender) {
	
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

	send(marker,packetOut,sender);
	// reset farid to 0!
	_farId=0;
}


// TODO make perhaps a FlowHandshake
UInt8 Handshake::handshakeHandler(UInt8 id,PacketReader& request,PacketWriter& response) {

	switch(id){
		case 0x30: {
			
			request.next8(); // passer un caractere (boite dans boite)
			UInt8 uriLen = request.next8()-1;
			if(request.next8() == 0x0a){ // TODO vérifier ça!
				string uri;
				request.readRaw(uriLen,uri);
				char tag[16];
				request.readRaw(tag,sizeof(tag));
	
				// RESPONSE 38
				response << (UInt8)sizeof(tag);
				response.writeRaw(tag,sizeof(tag));

				// New Cookie
				char cookie[65];
				RandomInputStream ris;
				ris.read(cookie,64);
				cookie[64]='\0';
				response.write8(64);
				response.writeRaw(cookie,64);
				_cookies[cookie] = new Cookie(uri);
				 
				// instance id (certificat in the middle)
				response.writeRaw(_certificat,sizeof(_certificat));
				
				return 0x70; 
			} else {
				if(_pOneMiddle) {
					// Just to make working the man in the middle mode
					PacketWriter req(6);
					request.reset(6);
					req.writeRaw(request.current(),request.available());
					PacketReader res = _pOneMiddle->requestCirrusHandshake(req);
					res.skip(3);
					UInt8 idResponse = res.next8();
					res.skip(2);
					response.writeRaw(res.current(),res.available());
					return idResponse;
				} else {
					// Normal mode
					// TODO
				}
				//Util::Dump(request.current(),request.available());
			}
			break;
		}
		case 0x38: {
			_farId = request.next32();
			string cookie;
			request.readRaw(request.next8(),cookie);

			map<string,Cookie*>::const_iterator itCookie = _cookies.find(cookie.c_str());
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
			
			// Compute Diffie-Hellman secret
			UInt8 pubKey[128];
			UInt8 sharedSecret[128];
			RTMFP::EndDiffieHellman(RTMFP::BeginDiffieHellman(pubKey),farPubKeyPart,sharedSecret);

			// Compute Keys
			UInt8 requestKey[AES_KEY_SIZE];
			UInt8 responseKey[AES_KEY_SIZE];
			RTMFP::ComputeAsymetricKeys(sharedSecret,requestKey,responseKey,pubKey,_signature,farCertificat);

			// RESPONSE
			
			_pNewSession = createSession(nextSessionId,_farId,itCookie->second->url,requestKey,responseKey);
			response << nextSessionId;
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


Session* Handshake::createSession(UInt32 id,UInt32 farId,const string& url,const UInt8* decryptKey,const UInt8* encryptKey) {
	if(!_listenCirrusUrl.empty()) {
		_pOneMiddle = new Middle(id,farId,url,decryptKey,encryptKey,_socket,_listenCirrusUrl);
		return _pOneMiddle;
	}
	return new Session(id,farId,url,decryptKey,encryptKey,_socket);
}




} // namespace Cumulus
