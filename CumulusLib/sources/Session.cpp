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

Session::Session(SendingEngine& sendingEngine,
				 UInt32 id,
				 UInt32 farId,
				 const Peer& peer,
				 const UInt8* decryptKey,
				 const UInt8* encryptKey) : 
		_pSendingThread(NULL),_sendingEngine(sendingEngine),flags(0),died(false),checked(false),id(id),farId(farId),peer(peer),aesDecrypt(decryptKey,AESEngine::DECRYPT),aesEncrypt(encryptKey,AESEngine::ENCRYPT),_pSocket(NULL),middleDump(false),_pSendingUnit(new SendingUnit()) {

}

Session::~Session() {
}

void Session::kill() {
	if(died)
		return;
	(bool&)died=true;
}

void Session::setEndPoint(Poco::Net::DatagramSocket& socket,const Poco::Net::SocketAddress& address) {
	if(_pSocket!=&socket)
		_pSocket=&socket;
	(SocketAddress&)peer.address = address;
}

void Session::receive(PacketReader& packet) {
	if(!RTMFP::Decode(decoder(),packet)) {
		ERROR("Decrypt error on session %u",id);
		return;
	}

	Logs::Dump(packet,format("Request from %s",peer.address.toString()).c_str(),middleDump);
	packetHandler(packet);
}

void Session::send() {
	if(!_pSocket) {
		ERROR("Impossible to send on a null socket for session %u",id);
		return;
	}
	send(farId,*_pSocket,peer.address);
}

void Session::send(UInt32 farId,DatagramSocket& socket,const SocketAddress& receiver) {
	Logs::Dump(_pSendingUnit->packet,6,format("Response to %s",receiver.toString()).c_str(),middleDump);
	

	_pSendingUnit->packet.limit(); // no limit for sending!
	_pSendingUnit->id = id;
	_pSendingUnit->farId = farId;
	_pSendingUnit->socket = socket;
	_pSendingUnit->address = receiver;
	_pSendingUnit->pAES = new AESEngine(encoder());
	_pSendingThread = _sendingEngine.enqueue(_pSendingUnit,_pSendingThread);
	_pSendingUnit = new SendingUnit();
}


} // namespace Cumulus
