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

#include "Session.h"
#include "Logs.h"
#include "Poco/Format.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Session::Session(ReceivingEngine& receivingEngine,
				 SendingEngine& sendingEngine,
				 UInt32 id,
				 UInt32 farId,
				 const Peer& peer,
				 const UInt8* decryptKey,
				 const UInt8* encryptKey) : 
	nextDumpAreMiddle(false),prevAESType(AESEngine::DEFAULT),_pSendingThread(NULL),_pReceivingThread(NULL),_receivingEngine(receivingEngine),_sendingEngine(sendingEngine),flags(0),died(false),checked(false),id(id),farId(farId),peer(peer),aesDecrypt(decryptKey,AESEngine::DECRYPT),aesEncrypt(encryptKey,AESEngine::ENCRYPT),_pRTMFPSending(new RTMFPSending()) {

}

Session::~Session() {

}

void Session::kill() {
	if(died)
		return;
	(bool&)died=true;
}

void Session::setEndPoint(Poco::Net::DatagramSocket& socket,const Poco::Net::SocketAddress& address) {
	_socket=socket;
	(SocketAddress&)peer.address = address;
}

void Session::decode(AutoPtr<RTMFPReceiving>& RTMFPReceiving,AESEngine::Type type) {
	prevAESType = type;
	RTMFPReceiving->decoder = aesDecrypt.next(type);
	try {
		_pReceivingThread = _receivingEngine.enqueue(RTMFPReceiving,_pReceivingThread);
	} catch(Exception& ex) {
		WARN("Receiving message impossible on session %u : %s",id,ex.displayText().c_str());
	}
}

void Session::receive(PacketReader& packet) {
	Logs::Dump(packet,format("Request from %s",peer.address.toString()).c_str(),nextDumpAreMiddle);
	packetHandler(packet);
}

void Session::send(UInt32 farId,DatagramSocket& socket,const SocketAddress& receiver,AESEngine::Type type) {
	Logs::Dump(_pRTMFPSending->packet,6,format("Response to %s",receiver.toString()).c_str(),nextDumpAreMiddle);
	nextDumpAreMiddle=false;
	
	_pRTMFPSending->packet.limit(); // no limit for sending!
	_pRTMFPSending->id = id;
	_pRTMFPSending->farId = farId;
	_pRTMFPSending->socket = socket;
	_pRTMFPSending->address = receiver;
	_pRTMFPSending->encoder = aesEncrypt.next(type);
	try {
		_pSendingThread = _sendingEngine.enqueue(_pRTMFPSending,_pSendingThread);
	} catch(Exception& ex) {
		WARN("Sending message refused on session %u : %s",id,ex.displayText().c_str());
	}
	_pRTMFPSending = new RTMFPSending();
}


} // namespace Cumulus
