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
#include "AMFWriter.h"
#include "AMFReader.h"
#include "Poco/RandomStream.h"
#include <openssl/evp.h>
#include "string.h"

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
				ServerHandler& serverHandler,
				const Sessions&	sessions,
				Target& target) : Session(id,farId,peer,decryptKey,encryptKey,socket,serverHandler),_pMiddleAesDecrypt(NULL),_pMiddleAesEncrypt(NULL),isPeer(target.isPeer),
					_middleId(0),_sessions(sessions),_firstResponse(false),_queryUrl("rtmfp://"+target.address.toString()+peer.path),_middlePeer(peer),_target(target) {

	Util::UnpackUrl(_queryUrl,(string&)_middlePeer.path,(map<string,string>&)_middlePeer.parameters);

	// connection to target
	_socket.connect(target.address);

	INFO("Handshake Target");

	memcpy(_middleCertificat,"\x02\x1D\x02\x41\x0E",5);
	RandomInputStream().read((char*)&_middleCertificat[5],64);
	memcpy(&_middleCertificat[69],"\x03\x1A\x02\x0A\x02\x1E\x02",7);

	////  HANDSHAKE TARGET  /////

	PacketWriter& packet = handshaker();

	if(target.isPeer) {
		_pMiddleDH = target.pDH;
		memcpy((UInt8*)_middlePeer.id,target.id,32);

		packet.write8(0x22);
		packet.write8(0x21);
		packet.write8(0x0F);
		packet.writeRaw(target.peerId,32);
	} else {
		packet.write8(_queryUrl.size()+2);
		packet.write8(_queryUrl.size()+1);
		packet.write8(0x0A);
		packet.writeRaw(_queryUrl);
	}

	packet.writeRandom(16); // tag

	sendHandshakeToTarget(0x30);
}

Middle::~Middle() {
	if(_pMiddleAesDecrypt)
		delete _pMiddleAesDecrypt;
	if(_pMiddleAesEncrypt)
		delete _pMiddleAesEncrypt;
}

PacketWriter& Middle::handshaker() {
	PacketWriter& writer(Session::writer());
	writer.clear(12);
	return writer;
}

void Middle::targetHandshakeHandler(UInt8 type,PacketReader& packet) {

	switch(type) {
		case 0x70: {

			string tag;
			packet.readString8(tag);

			// response	
			string cookie;
			packet.readString8(cookie);

			string targetCertificat;
			UInt8 nonce[KEY_SIZE+4]={0x81,0x02,0x1D,0x02};
			if(isPeer) {
				packet.next(4);
				memcpy(&nonce[4],_target.publicKey,KEY_SIZE);
				RTMFP::ComputeDiffieHellmanSecret(_pMiddleDH,packet.current(),_sharedSecret);
			} else {
				packet.readRaw(packet.available(),targetCertificat);
				_pMiddleDH = RTMFP::BeginDiffieHellman(&nonce[4]);
				EVP_Digest(nonce,sizeof(nonce),(UInt8*)_middlePeer.id,NULL,EVP_sha256(),NULL);
			}

			
			// request reply
			PacketWriter& request = handshaker();
			request << id(); // id session, we use the same that Cumulus id session for Flash client
			request.writeString8(cookie); // cookie
			request.write7BitValue(sizeof(nonce));
			request.writeRaw(nonce,sizeof(nonce));
			request.write7BitValue(sizeof(_middleCertificat));
			request.writeRaw(_middleCertificat,sizeof(_middleCertificat));
			request.write8(0x58);
			sendHandshakeToTarget(0x38);
			
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
				response.writeRaw(packet.current(),packet.available());

				// to send in handshake mode!
				UInt32 farId = _farId;
				_farId = 0;
				flush(SYMETRIC_ENCODING | WITHOUT_ECHO_TIME);
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
				kill(); // In this redirection request case, the target session has never existed!
			}

			break;
		}
		case 0x78: {

			// response
			packet >> _middleId;
			vector<UInt8> targetNonce(packet.read7BitValue());
			packet.readRaw(&targetNonce[0],targetNonce.size());
			
			if(!isPeer)
				RTMFP::EndDiffieHellman(_pMiddleDH,&targetNonce[targetNonce.size()-128],_sharedSecret);

			UInt8 requestKey[AES_KEY_SIZE];
			UInt8 responseKey[AES_KEY_SIZE];
			RTMFP::ComputeAsymetricKeys(_sharedSecret,_middleCertificat,sizeof(_middleCertificat),&targetNonce[0],targetNonce.size(),requestKey,responseKey);
			_pMiddleAesEncrypt = new AESEngine(requestKey,AESEngine::ENCRYPT);
			_pMiddleAesDecrypt = new AESEngine(responseKey,AESEngine::DECRYPT);
			break;
		}

		default: {
			ERROR("Unknown Target handshake type '%02x'",type);
		}
	}
}

void Middle::sendHandshakeToTarget(UInt8 type) {
	PacketWriter& packet(Session::writer());
	packet.reset(6);
	packet.write8(0x0b);
	packet << RTMFP::TimeNow();
	packet << type;
	packet.write16(packet.length()-packet.position()-2);

	Logs::Dump(packet,6,"Middle to Target handshaking:",true);

	RTMFP::Encode(packet);
	RTMFP::Pack(packet);
	_socket.sendBytes(packet.begin(),packet.length());
	writer(); // To delete the handshake response!
}

PacketWriter& Middle::requester() {
	PacketWriter& writer(Session::writer());
	writer.clear(6);
	return writer;
}

PacketWriter& Middle::writer() {
	PacketWriter& writer(Session::writer());
	writer.clear(11);
	return writer;
}

void Middle::packetHandler(PacketReader& packet) {
	if(!_pMiddleAesEncrypt) {
		DEBUG("500ms sleeping to wait target handshaking");
		Thread::sleep(500); // to wait the target handshake response
		manage();
	}

	// Middle to target
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

				if(!isPeer && idFlow==0x02 && stage==0x01) {
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
		sendToTarget();
}

void Middle::sendToTarget() {
	if(!_pMiddleAesEncrypt) {
		CRITIC("Send to target packet impossible because the middle hanshake has certainly failed, the target address is may be bad");
		return;
	}
	
	PacketWriter& packet(Session::writer());

	Logs::Dump(packet,6,"Middle to Target:",true);

	_firstResponse = true;
	RTMFP::Encode(*_pMiddleAesEncrypt,packet);
	RTMFP::Pack(packet,_middleId);
	_socket.sendBytes(packet.begin(),packet.length());
}

void Middle::targetPacketHandler(PacketReader& packet) {

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

	UInt8 idFlow,stage;
	UInt8 nbPeerSent = 0;

	while(type!=0xFF) {
		packetOut.write8(type);

		UInt16 size = packet.read16();
		PacketReader content(packet.current(),size);packetOut.write16(size);
		
		if(type==0x10 || type==0x11) {
			packetOut.write8(content.read8());
			if(type==0x10) {
				idFlow = content.read8();packetOut.write8(idFlow);
				stage = content.read8();packetOut.write8(stage);
			}
			if(stage==0x01 && ((marker==0x4e && idFlow==0x03) || (marker==0x8e && idFlow==0x05))) {
				/// Replace "middleId" by "peerId"
				
				if(type==0x10) {
					UInt8 tmp[10];
					content.readRaw(tmp,10);packetOut.writeRaw(tmp,10);
				} else
					packetOut.write8(content.read8());
				
				UInt8 middlePeerIdWanted[32];
				content.readRaw(middlePeerIdWanted,32);

				++nbPeerSent;

				Sessions::Iterator it;
				for(it=_sessions.begin();it!=_sessions.end();++it) {
					Middle* pMiddle = (Middle*)it->second;
					if(pMiddle->middlePeer() == middlePeerIdWanted) {
						memcpy(middlePeerIdWanted,pMiddle->peer().id,32);
						break;
					}
				}
				packetOut.writeRaw(middlePeerIdWanted,32);	

			}
		} else if(type == 0x0F) {
			packetOut.writeRaw(content.current(),3);content.next(3);
			UInt8 peerId[32];
			content.readRaw(peerId,32);

			if(memcmp(peerId,peer().id,32)!=0 && memcmp(peerId,_middlePeer.id,32)!=0)
				WARN("The p2pHandshake target packet doesn't match the peerId (or the middlePeerId)");
			// Replace by the peer.id
			packetOut.writeRaw(peer().id,32);
		}

		packetOut.writeRaw(content.current(),content.available());
		packet.next(size);

		type = packet.available()>0 ? packet.read8() : 0xFF;
	}

	if(nbPeerSent>0)
		INFO("%02x peers sending",nbPeerSent);

	int temp = packetOut.length();
	if(packetOut.length()>pos)
		flush();
}


void Middle::manage() {
	TRACE("Middle::manage");
	if(died())
		return;

	if (_socket.available()==0)
		return;

	int len = 0;
	try {
		len = _socket.receiveBytes(_buffer,sizeof(_buffer));
	} catch(Exception& ex) {
		ERROR("Middle socket reception error : %s",ex.displayText().c_str());
		return;
	}

	PacketReader packet(_buffer,len);

	if(packet.available()<RTMFP_MIN_PACKET_SIZE) {
		ERROR("Target to middle : invalid packet");
		return;
	}

	UInt32 id = RTMFP::Unpack(packet);

	// Handshaking
	if(id==0 || !_pMiddleAesDecrypt) {
		if(!RTMFP::Decode(packet)) {
			ERROR("Target handshake decrypt error");
			return;
		}

		Logs::Dump(packet,"Target to Middle handshaking:",true);

		UInt8 marker = packet.read8();
		if(marker!=0x0B) {
			ERROR("Target handshake received with a marker different of '0b'");
			return;
		}

		packet.read16(); // time

		UInt8 type = packet.read8();
		UInt16 size = packet.read16();
		PacketReader content(packet.current(),size);
		targetHandshakeHandler(type,content);
		return;
	}

	if(!RTMFP::Decode(*_pMiddleAesDecrypt,packet)) {
		ERROR("Target to middle : Decrypt error");
		return;
	}

	DEBUG("Target to middle : session d'identification '%u'",id);
	Logs::Dump(packet,"Target to Middle:",true);

	targetPacketHandler(packet);

}

void Middle::fail() {
	Session::fail();
	if(_pMiddleAesEncrypt) {
		PacketWriter& request = requester();
		request.write8(0x4a);
		request << RTMFP::TimeNow();
		request.write8(0x4c);
		request.write16(0);
		sendToTarget();
	}
}



} // namespace Cumulus
