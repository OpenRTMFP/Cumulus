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
#include "Poco/Format.h"
#include <openssl/evp.h>
#include "string.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Handshake::Handshake(ReceivingEngine& receivingEngine,SendingEngine& sendingEngine,Gateway& gateway,Handler& handler,Entity& entity) : ServerSession(receivingEngine,sendingEngine,0,0,Peer(handler),RTMFP_SYMETRIC_KEY,RTMFP_SYMETRIC_KEY,(Invoker&)handler),
	_gateway(gateway) {
	(bool&)checked=true;

	memcpy(_certificat,"\x01\x0A\x41\x0E",4);
	RandomInputStream().read((char*)&_certificat[4],64);
	memcpy(&_certificat[68],"\x02\x15\x02\x02\x15\x05\x02\x15\x0E",9);

	// Display far id flash side
	EVP_Digest(_certificat,sizeof(_certificat),(unsigned char *)entity.id,NULL,EVP_sha256(),NULL);
}


Handshake::~Handshake() {
	fail(""); // To avoid the failSignal
}

void Handshake::manage() {
	// delete obsolete cookie
	map<const UInt8*,Cookie*,CompareCookies>::iterator it=_cookies.begin();
	while(it!=_cookies.end()) {
		if(it->second->obsolete()) {
			eraseHelloAttempt(it->second->tag);
			DEBUG("Obsolete cookie : %s",Util::FormatHex(it->first,COOKIE_SIZE).c_str());
			delete it->second;
			_cookies.erase(it++);
		} else
			++it;
	}
}

void Handshake::commitCookie(const Session& session) {
	(bool&)session.checked = true;
	map<const UInt8*,Cookie*,CompareCookies>::iterator it;
	for(it=_cookies.begin();it!=_cookies.end();++it) {
		if(it->second->id==session.id) {
			eraseHelloAttempt(it->second->tag);
			delete it->second;
			_cookies.erase(it);
			return;
		}
	}
	WARN("Cookie of the session %u not found",session.id);
}

void Handshake::clear() {
	// delete cookies
	map<const UInt8*,Cookie*,CompareCookies>::const_iterator it;
	for(it=_cookies.begin();it!=_cookies.end();++it) {
		eraseHelloAttempt(it->second->tag);
		delete it->second;
	}
	_cookies.clear();
}

void Handshake::createCookie(PacketWriter& writer,HelloAttempt& attempt,const string& tag,const string& queryUrl) {
	// New Cookie
	Cookie* pCookie = attempt.pCookie;
	if(!pCookie) {
		if(attempt.pTarget)
			pCookie = new Cookie(tag,*attempt.pTarget);
		else
			pCookie = new Cookie(tag,queryUrl);
		_cookies[pCookie->value] =  pCookie;
		attempt.pCookie = pCookie;
	}
	writer.write8(COOKIE_SIZE);
	writer.writeRaw(pCookie->value,COOKIE_SIZE);
}


void Handshake::packetHandler(PacketReader& packet) {

	UInt8 marker = packet.read8();
	if(marker!=0x0b) {
		ERROR("Marker handshake wrong : should be 0b and not %u",marker);
		return;
	}
	
	UInt16 time = packet.read16();
	UInt8 id = packet.read8();
	packet.shrink(packet.read16()); // length

	PacketWriter& response(writer());
	UInt32 pos = response.position();
	response.next(3);
	UInt8 idResponse = handshakeHandler(id,packet,response);
	response.reset(pos);
	if(idResponse>0) {
		response.write8(idResponse);
		response.write16(response.length()-response.position()-2);
		flush();
	}

	// reset farid to 0!
	(UInt32&)farId=0;
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
				return _gateway.p2pHandshake(tag,response,peer.address,(const UInt8*)epd.c_str());

			if(type == 0x0a){
				/// Handshake
				HelloAttempt& attempt = helloAttempt<HelloAttempt>(tag);
				
				// New Cookie
				createCookie(response,attempt,tag,epd);

				// Fill peer infos
				UInt16 port;
				string host;
				Util::UnpackUrl(epd,host,port,(string&)peer.path,(map<string,string>&)peer.properties);
				set<string> addresses;
				peer.onHandshake(attempt.count+1,addresses);
				if(!addresses.empty()) {
					set<string>::iterator it;
					for(it=addresses.begin();it!=addresses.end();++it) {
						try {
							if((*it)=="again")
								*it = format("%s:%hu",host,port);
							SocketAddress address(*it);
							response.writeAddress(address,it==addresses.begin());
						} catch(Exception& ex) {
							ERROR("Bad redirection address %s in hello attempt, %s",(*it)=="again" ? epd.c_str() : (*it).c_str(),ex.displayText().c_str());
						}
					}
					return 0x71;
				}

				 
				// instance id (certificat in the middle)
				response.writeRaw(_certificat,sizeof(_certificat));
				return 0x70;
			} else
				ERROR("Unkown handshake first way with '%02x' type",type);
			break;
		}
		case 0x38: {
			(UInt32&)farId = request.read32();

			if(request.read7BitLongValue()!=COOKIE_SIZE) {
				ERROR("Bad handshake cookie '%s': its size should be 64 bytes",Util::FormatHex(request.current(),COOKIE_SIZE).c_str());
				return 0;
			}
	
			map<const UInt8*,Cookie*,CompareCookies>::iterator itCookie = _cookies.find(request.current());
			if(itCookie==_cookies.end()) {
				ERROR("Handshake cookie '%s' unknown",Util::FormatHex(request.current(),COOKIE_SIZE).c_str());
				return 0;
			}

			Cookie& cookie(*itCookie->second);

			if(cookie.id==0) {

				UInt8 decryptKey[AES_KEY_SIZE];UInt8* pDecryptKey=&decryptKey[0];
				UInt8 encryptKey[AES_KEY_SIZE];UInt8* pEncryptKey=&encryptKey[0];


				request.next(COOKIE_SIZE);
				size_t size = (size_t)request.read7BitLongValue();
				// peerId = SHA256(farPubKey)
				EVP_Digest(request.current(),size,(UInt8*)peer.id,NULL,EVP_sha256(),NULL);

				vector<UInt8> publicKey(request.read7BitValue()-2);
				request.next(2); // unknown
				request.readRaw(&publicKey[0],publicKey.size());

				size = request.read7BitValue();

				cookie.computeKeys(&publicKey[0],publicKey.size(),request.current(),(UInt16)size,decryptKey,encryptKey);

				// Fill peer infos
				Util::UnpackUrl(cookie.queryUrl,(string&)peer.path,(map<string,string>&)peer.properties);

				// RESPONSE
				Session& session = _gateway.createSession(farId,peer,pDecryptKey,pEncryptKey,cookie);
				(UInt32&)cookie.id = session.id;

				cookie.write();
			}
			if(cookie.response)
				cookie.read(response);

			return cookie.response;
		}
		default:
			ERROR("Unkown handshake packet id %u",id);
	}

	return 0;
}




} // namespace Cumulus
