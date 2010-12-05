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

#include "Middle.h"
#include "Logs.h"
#include "Util.h"
#include "RTMFP.h"
#include "Poco/RandomStream.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Middle::Middle(UInt32 id,
				UInt32 farId,
				const string& url,
				const Poco::UInt8* decryptKey,
				const Poco::UInt8* encryptKey,
				DatagramSocket& socket,
				const string& listenCirrusUrl) : Session(id,farId,url,decryptKey,encryptKey,socket),_middleCertificat("\x02\x1D\x02\x41\x0E",5),_cirrusUri(listenCirrusUrl),_pMiddleAesDecrypt(NULL),_pMiddleAesEncrypt(NULL),
					_handshakeDecrypt((UInt8*)RTMFP_SYMETRIC_KEY,AESEngine::DECRYPT),_handshakeEncrypt((UInt8*)RTMFP_SYMETRIC_KEY,AESEngine::ENCRYPT),_cirrusId(0) {

	INFO("Handshake Cirrus");;

	char rand[64];
	RandomInputStream().read(rand,sizeof(rand));
	_middleCertificat.append(rand,sizeof(rand));
	_middleCertificat.append("\x02\x15\x02\x02\x15\x05\x02\x15\x0E",9);

	// connection to cirrus
	_socketClient.connect(SocketAddress(_cirrusUri.getHost(),_cirrusUri.getPort()));

	////  HANDSHAKE CIRRUS  /////

	UInt8 type=0;

	PacketReader* pResponse = new PacketReader(_handshakeBuff,MAX_SIZE_MSG);
	do {
		PacketWriter request(6); // 6 for future crc 2 bytes and id session 4 bytes
		request.write8(0x0B);
		request << RTMFP::Timestamp();
		{
			PacketWriter content = request;
			content.skip(3);
			type = cirrusHandshakeHandler(type,*pResponse,content);
			if(type==0)
				break;
		}
		request.write8(type);
		request.write16(request.size()-request.position()-2);

		delete pResponse;
		pResponse = new PacketReader(requestCirrusHandshake(request));
		if(pResponse->available()==0)
			continue;

		pResponse->skip(3);
		type = pResponse->next8();
		int len = pResponse->next16(); // read real size content request
		pResponse->reset(pResponse->position(),len+12);
	} while(type>0);

	delete pResponse;
}

PacketReader Middle::requestCirrusHandshake(PacketWriter request) {
	Util::Dump(request,6,"dump.txt",true);
	RTMFP::Encode(_handshakeEncrypt,request);
	RTMFP::Pack(request,0);

	_socketClient.sendBytes(request.begin(),request.size());			// SEND
	int len = _socketClient.receiveBytes(_handshakeBuff,MAX_SIZE_MSG);	// RECEIVE
	
	PacketReader response(_handshakeBuff,len);

	if(RTMFP::IsValidPacket(response)) {
		RTMFP::Unpack(response);
		if(RTMFP::Decode(_handshakeDecrypt,response)) {
			Util::Dump(response,"dump.txt",true);
			return response;
		}
	}
	
	response.reset(0,0);
	return response;
}


Middle::~Middle() {
	if(_pMiddleAesDecrypt)
		delete _pMiddleAesDecrypt;
	if(_pMiddleAesEncrypt)
		delete _pMiddleAesEncrypt;
}

UInt8 Middle::cirrusHandshakeHandler(UInt8 type,PacketReader& response,PacketWriter request) {
	
	switch(type) {
		case 0x00: {
			// first request
			string uri = _cirrusUri.toString();
			request.write8(uri.size()+2);
			request.write8(uri.size()+1);
			request.write8(0x0A);
			request.writeRaw(uri);
			request.writeRandom(16);
			return 0x30;
		}
		case 0x70: {
			// response
			string tag;
			response.readString8(tag);
			string cookie;
			response.readString8(cookie);
			string cirrusCertificat;
			response.readRaw(response.available(),cirrusCertificat);

			// request reply
			UInt8 middlePubKey[128];
			_pMiddleDH = RTMFP::BeginDiffieHellman(middlePubKey);
			string middleSignature("\x81\x02\x1D\x02",4);
			
			request << id(); // id session, we use the same that Cumulus id session for Flash client
			request.writeString8(cookie); // cookie
			request.write8(0x81);
			request.writeString8(middleSignature);
			request.writeRaw((char*)middlePubKey,sizeof(middlePubKey)); // My public Key
			request.writeString8(_middleCertificat);
			request.write8(0x58);

			return 0x38;
		}
		case 0x78: {
			// response
			response >> _cirrusId;
			response.skip(1); // 0x81
			string cirrusSignature;
			response.readString8(cirrusSignature);
			UInt8 cirrusPubKey[128];
			response.readRaw((char*)cirrusPubKey,128);

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
			ERROR("Unknown Handshake type '%02x'",type);
		}
	}
	return 0;
	
}

void Middle::packetHandler(PacketReader& packet,SocketAddress& sender) {
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
	bool answer = true;
	{
		PacketWriter content = request;
		content.skip(5);
		if(marker==0x8D && type == 0x10 && idFlow==0x02 && stage==0x01) {
			// Replace http address
			content.writeRaw(packet.current(),87);packet.skip(87);
			string oldUrl;
			packet.readString8(oldUrl);
			string newUrl = _cirrusUri.toString();
			content.writeString8(newUrl); // new url
			length += newUrl.size()-oldUrl.size();
		}
		content.writeRaw(packet.current(),packet.available());
	}
	request << length;
	request << marker2;
	request << idFlow;
	request << stage;
	
	if(answer) {
		// Dump Flash to Cirrus
		Util::Dump(request,6,"dump.txt",true);

		RTMFP::Encode(*_pMiddleAesEncrypt,request);
		RTMFP::Pack(request,_cirrusId);
		_socketClient.sendBytes(request.begin(),request.size());
	}

	// Not anwser!
	if(marker == 0x0d && (type == 0x51 || type == 0x4c))
		return;

	// Cirrus to Middle
	UInt8 buff[MAX_SIZE_MSG];
	int len = _socketClient.receiveBytes(buff,MAX_SIZE_MSG);
	PacketReader response(buff,len);
	RTMFP::Unpack(response);
	if(!RTMFP::Decode(*_pMiddleAesDecrypt,response))
		ERROR("Decrypt error on cirrus to middle");

	PacketWriter packetOut(6);
	packetOut.writeRaw(response.current(),response.available());

	// Dump Cirrus to Flash
	Util::Dump(response,"dump.txt",true);

	send(*response.current(),packetOut,sender);
}


} // namespace Cumulus
