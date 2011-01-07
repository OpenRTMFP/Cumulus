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

#include "Middle.h"
#include "Logs.h"
#include "Util.h"
#include "RTMFP.h"
#include "Cirrus.h"
#include "Poco/RandomStream.h"
#include <openssl/evp.h>

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Middle::Middle(UInt32 id,
				UInt32 farId,
				const Peer& peer,
				const string& url,
				const UInt8* decryptKey,
				const UInt8* encryptKey,
				DatagramSocket& socket,
				ServerData& data,
				Cirrus& cirrus) : Session(id,farId,peer,url,decryptKey,encryptKey,socket,data),_middleCertificat("\x02\x1D\x02\x41\x0E",5),_pMiddleAesDecrypt(NULL),_pMiddleAesEncrypt(NULL),
					_cirrus(cirrus),_middleId(0),pPeerWanted(NULL),_firstResponse(false) {

	// connection to cirrus
	_socket.connect(_cirrus.address());
	

	INFO("Handshake Cirrus");

	char rand[64];
	RandomInputStream().read(rand,sizeof(rand));
	_middleCertificat.append(rand,sizeof(rand));
	_middleCertificat.append("\x03\x1A\x02\x0A\x02\x1E\x02",7);

	////  HANDSHAKE CIRRUS  /////

	PacketWriter& packet = handshaker();

	packet.write8(_cirrus.url().size()+2);
	packet.write8(_cirrus.url().size()+1);
	packet.write8(0x0A);
	packet.writeRaw(_cirrus.url());
	packet.writeRandom(16); // tag

	sendHandshakeToCirrus(0x30);
}

Middle::~Middle() {
	if(_pMiddleAesDecrypt)
		delete _pMiddleAesDecrypt;
	if(_pMiddleAesEncrypt)
		delete _pMiddleAesEncrypt;
}


PacketWriter& Middle::handshaker() {
	_packetOut.clear(12);
	return _packetOut;
}

void Middle::cirrusHandshakeHandler(UInt8 type,PacketReader& packet) {

	switch(type) {
		case 0x70: {

			string tag;
			packet.readString8(tag);

			// response	
			string cookie;
			packet.readString8(cookie);
			string cirrusCertificat;
			packet.readRaw(packet.available(),cirrusCertificat);

			// request reply
			string middleSignature("\x81\x02\x1D\x02",4);
			UInt8 middlePubKey[128];
			_pMiddleDH = RTMFP::BeginDiffieHellman(middlePubKey);

			string temp(middleSignature);
			temp.append((char*)middlePubKey,sizeof(middlePubKey));
			EVP_Digest(temp.c_str(),temp.size(),(UInt8*)_middlePeer.id,NULL,EVP_sha256(),NULL);
			

			PacketWriter& request = handshaker();
			request << id(); // id session, we use the same that Cumulus id session for Flash client
			request.writeString8(cookie); // cookie
			request.write8(0x81);
			request.writeString8(middleSignature);
			request.writeRaw((char*)middlePubKey,sizeof(middlePubKey)); // My public Key
			request.writeString8(_middleCertificat);
			request.write8(0x58);
			sendHandshakeToCirrus(0x38);
			
			break;
		}
		case 0x71: {

			string tag;
			packet.readString8(tag);

			if(_pMiddleAesDecrypt) {
				// P2P handshake
				PacketWriter& response = writer();
				response.write8(0x71);
				response.write16(packet.available()+tag.size()+1);
				response.writeString8(tag);
				response.write8(0x01);
				// replace public ip
				if(pPeerWanted) {
					response.writeAddress(pPeerWanted->address);
					packet.skip(7);
					pPeerWanted = NULL;
				}
				response.writeRaw(packet.current(),packet.available());

				// to send in handshake mode!
				UInt32 farId = _farId;
				_farId = 0;
				send(true);
				_farId = farId;

			}  else {
				// redirection request
				PacketReader content(packet);
				WARN("Man-in-middle mode leaks : redirection request, restart Cumulus with a url pertinant chooses among the following list");
				while(content.available()) {
				   if(content.next8()==0x01) {
					   UInt8 a=content.next8(),b=content.next8(),c=content.next8(),d=content.next8();
					   printf("%u.%u.%u.%u:%hu\n",a,b,c,d,content.next16());
				   }
				}
				 _die = true; // In this redirection request case, the session has never existed!
				 fail(); // to prevent the other side
			}

			break;
		}
		case 0x78: {

			// response
			packet >> _middleId;
			packet.skip(1); // 0x81
			string cirrusSignature;
			packet.readString8(cirrusSignature);
			UInt8 cirrusPubKey[128];
			packet.readRaw((char*)cirrusPubKey,sizeof(cirrusPubKey));

			UInt8 sharedSecret[128];
			RTMFP::EndDiffieHellman(_pMiddleDH,cirrusPubKey,sharedSecret);

			UInt8 requestKey[AES_KEY_SIZE];
			UInt8 responseKey[AES_KEY_SIZE];
			RTMFP::ComputeAsymetricKeys(sharedSecret,cirrusPubKey,cirrusSignature,_middleCertificat,requestKey,responseKey);
			_pMiddleAesEncrypt = new AESEngine(requestKey,AESEngine::ENCRYPT);
			_pMiddleAesDecrypt = new AESEngine(responseKey,AESEngine::DECRYPT);
			break;
		}

		default: {
			ERROR("Unknown Cirrus handshake type '%02x'",type);
		}
	}
}

void Middle::sendHandshakeToCirrus(UInt8 type) {
	_packetOut.reset(6);
	_packetOut.write8(0x0b);
	_packetOut << RTMFP::TimeNow();
	_packetOut << type;
	_packetOut.write16(_packetOut.size()-_packetOut.position()-2);

	if(Logs::Dump() && Logs::Middle()) {
		printf("Middle to Cirrus : handshake\n");
		Util::Dump(_packetOut,6,Logs::DumpFile());
	}

	RTMFP::Encode(_packetOut);
	RTMFP::Pack(_packetOut);
	_socket.sendBytes(_packetOut.begin(),_packetOut.size());
}

PacketWriter& Middle::requester() {
	_packetOut.clear(6);
	return _packetOut;
}

void Middle::packetHandler(PacketReader& packet) {
	if(!_pMiddleAesEncrypt) {
		DEBUG("500ms sleeping to wait cirrus handshaking");
		Thread::sleep(500); // to wait the cirrus handshake response
		manage();
	}

	_recvTimestamp.update();

	// Middle to cirrus
	PacketWriter& request = requester();

	UInt8 marker = packet.next8();
	request << marker;

	request << packet.next16();

	if((marker|0xF0) == 0xFD)
		request.write16(packet.next16()); // time echo

	int pos = request.position();

	UInt8 type = packet.available()>0 ? packet.next8() : 0xFF;
	while(type!=0xFF) {
		request<<type;

		UInt16 size = packet.next16();
		PacketReader content(packet.current(),size);

		{
			PacketWriter out(request);
			out.skip(2); // future size
			
			if(type==0x10 || type==0x11) {

				out.write8(content.next8());
				UInt8 idFlow = content.next8();out.write8(idFlow);
				UInt8 stage = content.next8();out.write8(stage);

				if(idFlow==0x02 && stage==0x01) {
					// Replace http address
					out.writeRaw(content.current(),14);content.skip(14);
					int temp = content.next16();out.write16(temp);
					out.writeRaw(content.current(),temp);content.skip(temp);

					out.writeRaw(content.current(),10);content.skip(10); // double

					temp = content.next16();out.write16(temp);++temp; // app
						out.writeRaw(content.current(),temp);content.skip(temp);
						temp = content.next16();out.write16(temp);
						out.writeRaw(content.current(),temp);content.skip(temp);

					temp = content.next16();out.write16(temp);++temp; // flashVer
						out.writeRaw(content.current(),temp);content.skip(temp);
						temp = content.next16();out.write16(temp);
						out.writeRaw(content.current(),temp);content.skip(temp);

					temp = content.next16();out.write16(temp); // swfUrl
						out.writeRaw(content.current(),temp);content.skip(temp);
						out.write8(content.next8());

					temp = content.next16();out.write16(temp);++temp; // tcUrl
						out.writeRaw(content.current(),temp);content.skip(temp);

					string oldUrl;
					content.readString16(oldUrl);
					out.writeString16(_cirrus.url()); // new url
					size += _cirrus.url().size()-oldUrl.size();
				}

			}  else if(type == 0x4C) {
				_die = true;
			}  else if(type == 0x51) {
				//&& idFlow==0x7F && stage == 0x02)
			}

			out.writeRaw(content.current(),content.available());
		}
		request.write16(size);
		packet.skip(size);

		type = packet.available()>0 ? packet.next8() : 0xFF;
	}
	
	if(request.size()>pos)
		sendToCirrus(_middleId);
}

void Middle::sendToCirrus(UInt32 id) {
	if(!_pMiddleAesEncrypt) {
		CRITIC("Send to cirrus packet impossible because the middle hanshake has certainly failed, restart Cumulus");
		return;
	}

	if(Logs::Dump() && Logs::Middle()) {
		printf("Middle to Cirrus\n");
		Util::Dump(_packetOut,6,Logs::DumpFile());
	}

	_firstResponse = true;
	RTMFP::Encode(*_pMiddleAesEncrypt,_packetOut);
	RTMFP::Pack(_packetOut,id);
	_socket.sendBytes(_packetOut.begin(),_packetOut.size());
}

void Middle::cirrusPacketHandler(PacketReader& packet) {

	if(_firstResponse)
		_recvTimestamp.update(); // To emulate a long ping corresponding, otherwise client send muyltiple times each packet
	_firstResponse = false;

	UInt8 marker = packet.next8();
	
	packet.next16(); // time

	if((marker|0xF0) == 0xFE)
		_timeSent = packet.next16(); // time echo

	PacketWriter& packetOut = writer();
	int pos = packetOut.position();

	UInt8 type = packet.available()>0 ? packet.next8() : 0xFF;

	while(type!=0xFF) {
		packetOut.write8(type);

		UInt16 size = packet.next16();
		PacketReader content(packet.current(),size);packetOut.write16(size);
		
		if(type==0x10 || type==0x11) {
			packetOut.write8(content.next8());
			UInt8 idFlow = content.next8();packetOut.write8(idFlow);
			UInt8 stage = content.next8();packetOut.write8(stage);
			if(stage==0x01 && ((marker==0x4e && idFlow==0x03) || (marker==0x8e && idFlow==0x05))) {
				// replace "middleId" by "peerId"
				packetOut.writeRaw(content.current(),10);content.skip(10);

				Peer middlePeer;
				content.readRaw((UInt8*)middlePeer.id,32);

				const Peer& peer = _cirrus.findPeer(middlePeer);
				packetOut.writeRaw(peer.id,32);

			}
		} else if(type == 0x0F) {
			// Stop the P2PHANDSHAKE CIRRUS PACKET!
			packetOut.clear(packetOut.position()-3);
			content.skip(content.available());
		}

		packetOut.writeRaw(content.current(),content.available());
		packet.skip(size);

		type = packet.available()>0 ? packet.next8() : 0xFF;
	}


	if(packetOut.size()>pos)
		send();
}


void Middle::manage() {
	TRACE("Middle::manage");
	if(die())
		return;

	if (!_socket.poll(_span, Socket::SELECT_READ))
		return;

	int len = 0;
	try {
		len = _socket.receiveBytes(_buffer,MAX_SIZE_MSG);
	} catch(Exception& ex) {
		ERROR("Middle socket reception error : %s",ex.displayText().c_str());
		return;
	}

	PacketReader packet(_buffer,len);

	if(!RTMFP::IsValidPacket(packet)) {
		ERROR("Cirrus to middle : invalid packet");
		return;
	}
	UInt32 id = RTMFP::Unpack(packet);

	// Handshaking
	if(id==0 || !_pMiddleAesDecrypt) {
		if(!RTMFP::Decode(packet)) {
			ERROR("Cirrus handshake decrypt error");
			return;
		}

		if(Logs::Dump() && Logs::Middle()) {
			printf("Cirrus to Middle : handshake\n");
			Util::Dump(packet,Logs::DumpFile());
		}

		UInt8 marker = packet.next8();
		if(marker!=0x0B) {
			ERROR("Cirrus handshake received with a marker different of '0b'");
			return;
		}

		packet.next16(); // time

		UInt8 type = packet.next8();
		UInt16 size = packet.next16();
		PacketReader content(packet.current(),size);
		cirrusHandshakeHandler(type,content);
		return;
	}

	if(!RTMFP::Decode(*_pMiddleAesDecrypt,packet)) {
		ERROR("Cirrus to middle : Decrypt error");
		return;
	}

	DEBUG("Cirrus to middle : session d'identification '%u'",id);
	if(Logs::Dump() && Logs::Middle()) {
		printf("Cirrus to Middle :\n");
		Util::Dump(packet,Logs::DumpFile());
	}

	cirrusPacketHandler(packet);

}



} // namespace Cumulus
