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
				const UInt8* peerId,
				const SocketAddress& peerAddress,
				const string& url,
				const UInt8* decryptKey,
				const UInt8* encryptKey,
				DatagramSocket& socket,
				ServerData& data,
				Cirrus& cirrus) : Session(id,farId,peerId,peerAddress,url,decryptKey,encryptKey,socket,data),_middleCertificat("\x02\x1D\x02\x41\x0E",5),_pMiddleAesDecrypt(NULL),_pMiddleAesEncrypt(NULL),
					_cirrus(cirrus),_middleId(0),pPeerAddressWanted(NULL) {

	// connection to cirrus
	_socket.connect(_cirrus.address());

	INFO("Handshake Cirrus");

	char rand[64];
	RandomInputStream().read(rand,sizeof(rand));
	_middleCertificat.append(rand,sizeof(rand));
	_middleCertificat.append("\x03\x1A\x02\x0A\x02\x1E\x02",7);

	////  HANDSHAKE CIRRUS  /////

	PacketWriter packet(12); // 6 for future crc 2 bytes and id session 4 bytes

	packet.write8(_cirrus.url().size()+2);
	packet.write8(_cirrus.url().size()+1);
	packet.write8(0x0A);
	packet.writeRaw(_cirrus.url());
	packet.writeRandom(16); // tag

	sendHandshakeToCirrus(0x30,packet);
}

Middle::~Middle() {
	if(_pMiddleAesDecrypt)
		delete _pMiddleAesDecrypt;
	if(_pMiddleAesEncrypt)
		delete _pMiddleAesEncrypt;
}


void Middle::packetHandler(PacketReader& packet) {
	if(!_pMiddleAesEncrypt) {
		Sleep(500); // to wait the cirrus handshake response
		manage();
	}

	// Middle to cirrus
	PacketWriter request;
	request.skip(6);
	UInt8 marker = packet.next8();request<<marker;
	request.writeRaw(packet.current(),4);packet.skip(4);
	UInt8 type = packet.next8();request<<type;
	UInt16 length = packet.next16();
	UInt8 marker2 = packet.next8();
	UInt8 idFlow = packet.next8();
	UInt8 stage = packet.next8();
	bool send = true;
	{
		PacketWriter content = request;
		content.skip(5);
		if(marker==0x8D && type == 0x10 && idFlow==0x02 && stage==0x01) {
			// Replace http address
			content.writeRaw(packet.current(),14);packet.skip(14);
			int temp = packet.next16();content.write16(temp);
			content.writeRaw(packet.current(),temp);packet.skip(temp);

			content.writeRaw(packet.current(),10);packet.skip(10); // double

			temp = packet.next16();content.write16(temp);++temp; // app
				content.writeRaw(packet.current(),temp);packet.skip(temp);
				temp = packet.next16();content.write16(temp);
				content.writeRaw(packet.current(),temp);packet.skip(temp);

			temp = packet.next16();content.write16(temp);++temp; // flashVer
				content.writeRaw(packet.current(),temp);packet.skip(temp);
				temp = packet.next16();content.write16(temp);
				content.writeRaw(packet.current(),temp);packet.skip(temp);

			temp = packet.next16();content.write16(temp); // swfUrl
				content.writeRaw(packet.current(),temp);packet.skip(temp);
				content.write8(packet.next8());

			temp = packet.next16();content.write16(temp);++temp; // tcUrl
				content.writeRaw(packet.current(),temp);packet.skip(temp);

			string oldUrl;
			packet.readString16(oldUrl);
			content.writeString16(_cirrus.url()); // new url
			length += _cirrus.url().size()-oldUrl.size();
		}// else if(marker == 0x0d && type == 0x51 && idFlow==0x7F && stage == 0x02)
		//	send = false;
		content.writeRaw(packet.current(),packet.available());
	}
	request << length;
	request << marker2;
	request << idFlow;
	request << stage;
	
	if(send)
		sendToCirrus(_middleId,request);
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

			UInt8 middlePeerId[32];
			string temp(middleSignature);
			temp.append((char*)middlePubKey,sizeof(middlePubKey));
			EVP_Digest(temp.c_str(),temp.size(),middlePeerId,NULL,EVP_sha256(),NULL);
			_middlePeerId.assignRaw(middlePeerId,32);
			

			PacketWriter request(12);
			request << id(); // id session, we use the same that Cumulus id session for Flash client
			request.writeString8(cookie); // cookie
			request.write8(0x81);
			request.writeString8(middleSignature);
			request.writeRaw((char*)middlePubKey,sizeof(middlePubKey)); // My public Key
			request.writeString8(_middleCertificat);
			request.write8(0x58);
			sendHandshakeToCirrus(0x38,request);
			
			break;
		}
		case 0x71: {
			
			string tag;
			packet.readString8(tag);

			if(_pMiddleAesDecrypt) {
				// P2P handshake
				PacketWriter response(9);
				response.write8(0x71);
				response.write16(packet.available()+17);
				response.writeString8(tag);
				response.write8(0x01);
				// replace public ip
				if(pPeerAddressWanted) {
					response.writeAddress(*pPeerAddressWanted);
					packet.skip(7);
					pPeerAddressWanted = NULL;
				}
				response.writeRaw(packet.current(),packet.available());

				// to send in handshake mode!
				UInt32 farId = _farId;
				_farId = 0;
				send(0x0b,response);
				_farId = farId;
				break;
			} 

			// redirection request
			CRITIC("Mode man-in-middle impossible! (redirection request)");
			printf("Restart Cumulus with a url pertinant chooses among the following list:\n");
			while(packet.available()) {
			   if(packet.next8()==0x01) {
				   UInt8 a=packet.next8(),b=packet.next8(),c=packet.next8(),d=packet.next8();
				   printf("%u.%u.%u.%u:%hu\n",a,b,c,d,packet.next16());
			   }
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
			RTMFP::ComputeAsymetricKeys(sharedSecret,requestKey,responseKey,cirrusPubKey,cirrusSignature,_middleCertificat);
			_pMiddleAesEncrypt = new AESEngine(requestKey,AESEngine::ENCRYPT);
			_pMiddleAesDecrypt = new AESEngine(responseKey,AESEngine::DECRYPT);
			break;
		}

		default: {
			ERROR("Unknown Cirrus handshake type '%02x'",type);
		}
	}
}

void Middle::cirrusPacketHandler(PacketReader& packet) {

	PacketWriter packetOut(9);
	UInt8 marker = packet.next8();packet.skip(2);

	if(! ((marker|0xF0) == 0xFA) ) {
		packetOut.writeRaw(packet.current(),2);packet.skip(2);

		UInt8 type = packet.available()>0 ? packet.next8() : 0xFF;

		while(type!=0xFF) {
			packetOut.write8(type);

			UInt16 size = packet.next16();
			PacketReader content(packet.current(),size);packetOut.write16(size);
			
			if(type==0x10) {
				packetOut.write8(content.next8());
				UInt8 idFlow = content.next8();packetOut.write8(idFlow);
				UInt8 stage = content.next8();packetOut.write8(stage);
				if(stage==0x01 && ((marker==0x4e && idFlow==0x03) || (marker==0x8e && idFlow==0x05))) {
					// replace "middleId" by "peerId"
					packetOut.writeRaw(content.current(),10);content.skip(10);
					UInt8 temp[32];
					content.readRaw(temp,sizeof(temp));

					const BLOB& peerId = _cirrus.findPeerId(BLOB(temp,sizeof(temp)));
					packetOut.writeRaw(peerId.begin(),peerId.size());

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
	} else
		packetOut.writeRaw(packet.current(),packet.available());

	if(packetOut.size()>11)
		send(marker,packetOut);
}


void Middle::sendHandshakeToCirrus(UInt8 type,PacketWriter& request) {
	request.reset(6);
	request.write8(0x0b);
	request << RTMFP::Timestamp();
	request << type;
	request.write16(request.size()-request.position()-2);

	//printf("Middle to Cirrus : handshake\n");
	//Util::Dump(request,6);

	RTMFP::Encode(request);
	RTMFP::Pack(request);
	_socket.sendBytes(request.begin(),request.size());
}

void Middle::sendToCirrus(UInt32 id,PacketWriter& request) {
	if(!_pMiddleAesEncrypt) {
		CRITIC("Send to cirrus packet impossible because the middle hanshake has certainly failed, restart Cumulus");
		return;
	}

	//printf("Middle to Cirrus\n");
	//Util::Dump(request,6);

	RTMFP::Encode(*_pMiddleAesEncrypt,request);
	RTMFP::Pack(request,id);
	_socket.sendBytes(request.begin(),request.size());
}


bool Middle::manage() {
	TRACE("Cirrus::process");

	if (!_socket.poll(_span, Socket::SELECT_READ))
		return true;

	int len = 0;
	try {
		len = _socket.receiveBytes(_buffer,MAX_SIZE_MSG);
	} catch(Exception& ex) {
		ERROR("Middle socket reception error : %s",ex.displayText().c_str());
		return true;
	}

	PacketReader packet(_buffer,len);

	if(!RTMFP::IsValidPacket(packet)) {
		ERROR("Cirrus to middle : invalid packet");
		return true;
	}
	UInt32 id = RTMFP::Unpack(packet);

	// Handshaking
	if(id==0 || !_pMiddleAesDecrypt) {
		if(!RTMFP::Decode(packet)) {
			ERROR("Cirrus handshake decrypt error");
			return true;
		}

		//printf("Cirrus to Middle : handshake\n");
		//Util::Dump(packet);

		UInt8 marker = packet.next8();
		if(marker!=0x0B) {
			ERROR("Cirrus handshake received with a marker different of '0b'");
			return true;
		}
		packet.skip(2);
		UInt8 type = packet.next8();
		UInt16 size = packet.next16();
		cirrusHandshakeHandler(type,PacketReader(packet.current(),size));
		return true;
	}

	if(!RTMFP::Decode(*_pMiddleAesDecrypt,packet)) {
		ERROR("Cirrus to middle : Decrypt error");
		return true;
	}

	DEBUG("Cirrus to middle : session d'identification '%u'",id);
//	printf("Cirrus to Middle :\n");
//	Util::Dump(packet);

	cirrusPacketHandler(packet);
	
	return true;
}



} // namespace Cumulus
