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
#include "AMFWriter.h"
#include "AMFReader.h"
#include "Poco/RandomStream.h"
#include <openssl/evp.h>

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Middle::Middle(UInt32 id,
				UInt32 farId,
				const Peer& peer,
				const UInt8* decryptKey,
				const UInt8* encryptKey,
				DatagramSocket& socket,
				ServerData& data,
				Cirrus& cirrus) : Session(id,farId,peer,decryptKey,encryptKey,socket,data),_middleCertificat("\x02\x1D\x02\x41\x0E",5),_pMiddleAesDecrypt(NULL),_pMiddleAesEncrypt(NULL),
					_cirrus(cirrus),_middleId(0),pPeerWanted(NULL),_firstResponse(false),_queryUrl("rtmfp://"+cirrus.address().toString()+peer.path) {

	Util::UnpackUrl(_queryUrl,(string&)_middlePeer.path,(map<string,string>&)_middlePeer.parameters);

	// connection to cirrus
	_socket.connect(_cirrus.address());
	

	INFO("Handshake Cirrus");

	char rand[64];
	RandomInputStream().read(rand,sizeof(rand));
	_middleCertificat.append(rand,sizeof(rand));
	_middleCertificat.append("\x03\x1A\x02\x0A\x02\x1E\x02",7);

	////  HANDSHAKE CIRRUS  /////

	PacketWriter& packet = handshaker();

	packet.write8(_queryUrl.size()+2);
	packet.write8(_queryUrl.size()+1);
	packet.write8(0x0A);
	packet.writeRaw(_queryUrl);
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
					packet.next(7);
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
				   if(content.read8()==0x01) {
					   UInt8 a=content.read8(),b=content.read8(),c=content.read8(),d=content.read8();
					   printf("%u.%u.%u.%u:%hu",a,b,c,d,content.read16());
					   cout << endl;
				   }
				}
				
				Session::fail("Redirection 'man in the middle' request"); // to prevent the other side
				kill(); // In this redirection request case, the cirrus session has never existed!
			}

			break;
		}
		case 0x78: {

			// response
			packet >> _middleId;
			packet.next(1); // 0x81
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
	_packetOut.write16(_packetOut.length()-_packetOut.position()-2);

	if(Logs::Dump() && Logs::Middle()) {
		cout << "Middle to Cirrus handshaking:" << endl;
		Util::Dump(_packetOut,6,Logs::DumpFile());
	}

	RTMFP::Encode(_packetOut);
	RTMFP::Pack(_packetOut);
	_socket.sendBytes(_packetOut.begin(),_packetOut.length());
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

	// Middle to cirrus
	PacketWriter& request = requester();

	UInt8 marker = packet.read8();
	request << marker;

	request << packet.read16();

	if((marker|0xF0) == 0xFD)
		request.write16(packet.read16()); // time echo

	int pos = request.position();

	UInt8 type = packet.available()>0 ? packet.read8() : 0xFF;
	while(type!=0xFF) {
		UInt16 oldSize = packet.read16();
		UInt16 newSize = oldSize;
		PacketReader content(packet.current(),oldSize);

		{
			PacketWriter out(request); // 3 for future type and size
			out.next(3);
			
			if(type==0x10) {

				out.write8(content.read8());
				UInt8 idFlow = content.read8();out.write8(idFlow);
				UInt8 stage = content.read8();out.write8(stage);

				if(idFlow==0x02 && stage==0x01) {
					/// Replace NetConnection infos

					out.writeRaw(content.current(),14);content.next(14);

					// first string
					string tmp;
					content.readString16(tmp);out.writeString16(tmp);

					AMFWriter writer(out);
					AMFReader reader(content);
					writer.writeNumber(reader.readNumber()); // double
					
					AMFObject obj;
					reader.readObject(obj);
					
					/// Replace tcUrl
					if(obj.has("tcUrl")) {
						string oldUrl = obj.getString("tcUrl");
						obj.setString("tcUrl",_queryUrl);
						newSize += _queryUrl.size()-oldUrl.size();
					}
					
					writer.writeObject(obj);

				}

			}  else if(type == 0x4C) {
				 kill();
			}
			out.writeRaw(content.current(),content.available());
		}
		if((request.length()-request.position())==(newSize+3)) {
			request<<type;
			request.write16(newSize);request.next(newSize);
		}

		packet.next(oldSize);
		type = packet.available()>0 ? packet.read8() : 0xFF;
	}

	if(request.length()>pos)
		sendToCirrus();
}

void Middle::sendToCirrus() {
	if(!_pMiddleAesEncrypt) {
		CRITIC("Send to cirrus packet impossible because the middle hanshake has certainly failed, restart Cumulus");
		return;
	}

	if(Logs::Dump() && Logs::Middle()) {
		cout << "Middle to Cirrus:" << endl;
		Util::Dump(_packetOut,6,Logs::DumpFile());
	}

	_firstResponse = true;
	RTMFP::Encode(*_pMiddleAesEncrypt,_packetOut);
	RTMFP::Pack(_packetOut,_middleId);
	_socket.sendBytes(_packetOut.begin(),_packetOut.length());
}

void Middle::cirrusPacketHandler(PacketReader& packet) {

	if(_firstResponse)
		_recvTimestamp.update(); // To emulate a long ping corresponding, otherwise client send muyltiple times each packet
	_firstResponse = false;

	UInt8 marker = packet.read8();
	
	packet.read16(); // time

	if((marker|0xF0) == 0xFE)
		_timeSent = packet.read16(); // time echo

	PacketWriter& packetOut = writer();
	int pos = packetOut.position();

	UInt8 type = packet.available()>0 ? packet.read8() : 0xFF;

	while(type!=0xFF) {
		packetOut.write8(type);

		UInt16 size = packet.read16();
		PacketReader content(packet.current(),size);packetOut.write16(size);
		
		if(type==0x10) {
			packetOut.write8(content.read8());
			UInt8 idFlow = content.read8();packetOut.write8(idFlow);
			UInt8 stage = content.read8();packetOut.write8(stage);
			if(stage==0x01 && ((marker==0x4e && idFlow==0x03) || (marker==0x8e && idFlow==0x05))) {
				/// Replace "middleId" by "peerId"

				UInt8 tmp[10];
				content.readRaw(tmp,10);packetOut.writeRaw(tmp,10);
				
				UInt8 middlePeerIdWanted[32];
				content.readRaw(middlePeerIdWanted,32);

				const Middle* pMiddleWanted = _cirrus.findMiddle(middlePeerIdWanted);

				if(!pMiddleWanted)
					packetOut.writeRaw(middlePeerIdWanted,32);
				else
					packetOut.writeRaw(pMiddleWanted->peer().id,32);

			}
		} else if(type == 0x0F) {
			// Stop the P2PHANDSHAKE CIRRUS PACKET!
			packetOut.clear(packetOut.position()-3);
			content.next(content.available());
			NOTE("UDP Hole punching cirrus packet annihilated")
		}

		packetOut.writeRaw(content.current(),content.available());
		packet.next(size);

		type = packet.available()>0 ? packet.read8() : 0xFF;
	}

	if(packetOut.length()>pos)
		send();
}


void Middle::manage() {
	TRACE("Middle::manage");
	if(die())
		return;

	if (_socket.available()==0)
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
			cout << "Cirrus to Middle handshaking:" << endl;
			Util::Dump(packet,Logs::DumpFile());
		}

		UInt8 marker = packet.read8();
		if(marker!=0x0B) {
			ERROR("Cirrus handshake received with a marker different of '0b'");
			return;
		}

		packet.read16(); // time

		UInt8 type = packet.read8();
		UInt16 size = packet.read16();
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
		cout << "Cirrus to Middle:" << endl;
		Util::Dump(packet,Logs::DumpFile());
	}

	cirrusPacketHandler(packet);

}

void Middle::fail() {
	Session::fail();
	PacketWriter& request = requester();
	request.write8(0x4a);
	request << RTMFP::TimeNow();
	request.write8(0x4c);
	request.write16(0);
	sendToCirrus();
}



} // namespace Cumulus
