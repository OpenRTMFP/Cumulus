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

#pragma once

#include "Cumulus.h"
#include "PacketReader.h"
#include "PacketWriter.h"
#include "AESEngine.h"
#include "RTMFP.h"
#include "Peer.h"
#include "SendingEngine.h"
#include "Poco/Net/DatagramSocket.h"

namespace Cumulus {

class Session {
public:

	Session(SendingEngine&	sendingEngine,
			Poco::UInt32 id,
			Poco::UInt32 farId,
			const Peer& peer,
			const Poco::UInt8* decryptKey,
			const Poco::UInt8* encryptKey);

	virtual ~Session();

	const Poco::UInt32	id;
	const Poco::UInt32	farId;
	Peer				peer;
	const bool			checked;
	const bool			died;

	Poco::UInt8			flags; // Allow to children class to save some flags (see ServerSession and SESSION_BY_EDGE)

	bool				middleDump;

	virtual void		manage(){}

	void				setEndPoint(Poco::Net::DatagramSocket& socket,const Poco::Net::SocketAddress& address);
	void				receive(PacketReader& packet);
	void				send();
	PacketWriter&		writer();
	virtual void		kill();

protected:
	void				send(Poco::UInt32 farId,Poco::Net::DatagramSocket& socket,const Poco::Net::SocketAddress& receiver);

	AESEngine			aesDecrypt;
	AESEngine			aesEncrypt;

private:
	virtual AESEngine	decoder();
	virtual AESEngine	encoder();

	virtual void	packetHandler(PacketReader& packet)=0;
	
	SendingEngine&				_sendingEngine;
	Poco::AutoPtr<SendingUnit>	_pSendingUnit;
	Poco::Net::DatagramSocket	_socket;
	SendingThread*				_pSendingThread;
};

inline void Session::send() {
	send(farId,_socket,peer.address);
}


inline PacketWriter& Session::writer() {
	return _pSendingUnit->packet;
}

inline AESEngine Session::decoder() {
	return aesDecrypt.next();
}

inline AESEngine Session::encoder() {
	return aesEncrypt.next();
}


} // namespace Cumulus
