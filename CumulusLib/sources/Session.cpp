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
#include "Handshake.h"
#include "Poco/Format.h"
#include <errno.h> // TODO remove this line!!

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace Cumulus {

Session::Session(UInt32 id,
				 UInt32 farId,
				 const Peer& peer,
				 const UInt8* decryptKey,
				 const UInt8* encryptKey,
				 Invoker& invoker) :
	invoker(invoker),nextDumpAreMiddle(false),_prevAESType(AESEngine::DEFAULT),_pSendingThread(NULL),_pReceivingThread(NULL),died(false),checked(false),id(id),farId(farId),peer(peer),aesDecrypt(decryptKey,AESEngine::DECRYPT),aesEncrypt(encryptKey,AESEngine::ENCRYPT),_pRTMFPSending(new RTMFPSending()) {
	(*this->peer.addresses.begin())= peer.address.toString();
}

Session::~Session() {

}

void Session::kill() {
	if(died)
		return;
	(bool&)died=true;
}

bool Session::setEndPoint(Poco::Net::DatagramSocket& socket,const Poco::Net::SocketAddress& address) {
	_socket=socket;
	if(address.host()==peer.address.host() && address.port()==peer.address.port())
		return false;
	(SocketAddress&)peer.address = address;
	(*peer.addresses.begin())=address.toString();
	return true;
}

void Session::decode(AutoPtr<RTMFPReceiving>& pRTMFPReceiving,AESEngine::Type type) {
	pRTMFPReceiving->decoder = aesDecrypt.next(type);
	try {
		_pReceivingThread = invoker.poolThreads.enqueue(pRTMFPReceiving.cast<WorkThread>(),_pReceivingThread);
	} catch(Exception& ex) {
		WARN("Receiving message impossible on session %u : %s",id,ex.displayText().c_str());
		ERROR("Error %d : %s", errno, strerror(errno)); // TODO remove this line!!
	}
	ScopedLock<FastMutex> lock(_mutex);
	_prevAESType = type;
}

void Session::receive(PacketReader& packet) {
	if(nextDumpAreMiddle)
		DUMP_MIDDLE(packet,format("Request from %s",peer.address.toString()).c_str())
	else
		DUMP(packet,format("Request from %s",peer.address.toString()).c_str())
	((Timestamp&)peer.lastReceptionTime).update();
	packetHandler(packet);
}

void Session::send(UInt32 farId,DatagramSocket& socket,const SocketAddress& receiver,AESEngine::Type type) {
	if(nextDumpAreMiddle)
		DUMP_MIDDLE(_pRTMFPSending->packet,6,format("Response to %s",receiver.toString()).c_str())
	else
		DUMP(_pRTMFPSending->packet,6,format("Response to %s",receiver.toString()).c_str())
	nextDumpAreMiddle=false;
	
	_pRTMFPSending->packet.limit(); // no limit for sending!
	_pRTMFPSending->id = id;
	_pRTMFPSending->farId = farId;
	_pRTMFPSending->pSocket = new DatagramSocket(socket);
	_pRTMFPSending->address = receiver;
	_pRTMFPSending->encoder = aesEncrypt.next(type);
	try {
		_pSendingThread = invoker.poolThreads.enqueue(_pRTMFPSending.cast<WorkThread>(),_pSendingThread);
	} catch(Exception& ex) {
		WARN("Sending message refused on session %u : %s",id,ex.displayText().c_str());
	}
	_pRTMFPSending = new RTMFPSending();
}


} // namespace Cumulus
