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
#include "string.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Handshake::Handshake(Gateway& gateway,Handler& handler,Entity& entity) : ServerSession(0,0,Peer(handler),RTMFP_SYMETRIC_KEY,RTMFP_SYMETRIC_KEY,(Invoker&)handler),
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

void Handshake::commitCookie(const UInt8* value) {
	map<const UInt8*,Cookie*,CompareCookies>::iterator it = _cookies.find(value);
	if(it==_cookies.end()) {
		WARN("Cookie %s not found, maybe becoming obsolete before commiting (congestion?)",Util::FormatHex(value,COOKIE_SIZE).c_str());
		return;
	}
	eraseHelloAttempt(it->second->tag);
	delete it->second;
	_cookies.erase(it);
	return;
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
			pCookie = new Cookie(invoker,tag,*attempt.pTarget);
		else
			pCookie = new Cookie(*this,invoker,tag,queryUrl);
		_cookies[pCookie->value()] =  pCookie;
		attempt.pCookie = pCookie;
	}
	writer.write8(COOKIE_SIZE);
	writer.writeRaw(pCookie->value(),COOKIE_SIZE);
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

Session* Handshake::createSession(const UInt8* cookieValue) {
	map<const UInt8*,Cookie*,CompareCookies>::iterator itCookie = _cookies.find(cookieValue);
	if(itCookie==_cookies.end()) {
		WARN("Creating session for an unknown cookie '%s' (CPU congestion?)",Util::FormatHex(cookieValue,COOKIE_SIZE).c_str());
		return NULL;
	}

	Cookie& cookie(*itCookie->second);

	// Fill peer infos
	memcpy((void*)peer.id,cookie.peerId,ID_SIZE);
	Util::UnpackUrl(cookie.queryUrl,(string&)peer.path,(map<string,string>&)peer.properties);
	(UInt32&)farId = cookie.farId;
	(SocketAddress&)peer.address = cookie.peerAddress;

	// Create session and write cookie
	Session& session = _gateway.createSession(peer,cookie);
	(UInt32&)cookie.id = session.id;
	cookie.write();

	// Just for middle mode!
	if(cookie.pTarget) {
		((vector<UInt8>&)cookie.pTarget->initiatorNonce).resize(cookie.initiatorNonce().size());
		memcpy(&((vector<UInt8>&)cookie.pTarget->initiatorNonce)[0],&cookie.initiatorNonce()[0],cookie.initiatorNonce().size());
		(vector<UInt8>&)cookie.pTarget->sharedSecret = cookie.sharedSecret();
	}

	// response!
	PacketWriter& response(writer());
	response.write8(0x78);
	response.write16(cookie.length());
	cookie.read(response);
	flush();
	(UInt32&)farId=0; // reset farid to 0!
	return &session;
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
								((string&)*it).assign(format("%s:%hu",host,port));
							SocketAddress address(*it);
							response.writeAddress(address,it==addresses.begin());
						} catch(Exception& ex) {
							ERROR("Bad redirection address %s in hello attempt, %s",(*it)=="again" ? epd.c_str() : (*it).c_str(),ex.displayText().c_str());
						}
					}
					return 0x71;
				}

				// New Cookie
				createCookie(response,attempt,tag,epd);

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
				WARN("Cookie %s unknown, maybe already connected (udpBuffer congested?)",Util::FormatHex(request.current(),COOKIE_SIZE).c_str());
				return 0;
			}

			Cookie& cookie(*itCookie->second);
			(SocketAddress&)cookie.peerAddress = peer.address;

			if(cookie.farId==0) {
				((UInt32&)cookie.farId) = farId;
				request.next(COOKIE_SIZE);

				size_t size = (size_t)request.read7BitLongValue();
				// peerId = SHA256(farPubKey)
				EVP_Digest(request.current(),size,(UInt8*)cookie.peerId,NULL,EVP_sha256(),NULL);

				cookie.initiatorKey().resize(request.read7BitValue()-2);
				request.next(2); // unknown
				request.readRaw(&cookie.initiatorKey()[0],cookie.initiatorKey().size());

				cookie.initiatorNonce().resize(request.read7BitValue());
				request.readRaw(&cookie.initiatorNonce()[0],cookie.initiatorNonce().size());

				cookie.computeKeys();
			} else if(cookie.id>0) {
				// Repeat cookie reponse!
				cookie.read(response);
				return 0x78;
			} // else Keys are computing (multi-thread)

			break;
		}
		default:
			ERROR("Unkown handshake packet id %u",id);
	}

	return 0;
}




} // namespace Cumulus
