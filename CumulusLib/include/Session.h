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
#include "Flow.h"
#include "Poco/Timestamp.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/DatagramSocket.h"

namespace Cumulus {


class Session
{
public:

	Session(Poco::UInt32 id,
			Poco::UInt32 farId,
			const Poco::UInt8* peerId,
			const Poco::Net::SocketAddress& peerAddress,
			const std::string& url,
			const Poco::UInt8* decryptKey,
			const Poco::UInt8* encryptKey,
			Poco::Net::DatagramSocket& socket,
			Database& database);

	virtual ~Session();

	virtual void	packetHandler(PacketReader& packet);
	bool			decode(PacketReader& packet);

	Poco::UInt32	id() const;
	Poco::UInt32	farId() const;

	const BLOB&						peerId();
	const Poco::Net::SocketAddress&	peerAddress();

	virtual void	p2pHandshake(const Poco::Net::SocketAddress& peerAddress);

protected:

	void				send(Poco::UInt8 marker,PacketWriter packet);

	Poco::UInt32				_farId; // Protected for Handshake session
	Poco::Net::DatagramSocket&	_socket; // Protected for Handshake session
	Poco::Net::SocketAddress	_peerAddress; // Protected for Handshake session
	Database&					_database; // Protected for Handshake session
	AESEngine					_aesDecrypt; // Protected for Handshake session
	AESEngine					_aesEncrypt; // Protected for Handshake session
private:
	
	Flow*				createFlow(Poco::UInt8 id);
	Flow&				flow(Poco::UInt8 id);
	

	void				keepaliveHandler();

	std::map<Poco::UInt8,Flow*> _flows;	

	std::string					_url;
	Poco::UInt32				_id;
	BLOB						_peerId;
};

inline const BLOB& Session::peerId() {
	return _peerId;
}

inline const Poco::Net::SocketAddress& Session::peerAddress() {
	return _peerAddress;
}

inline bool Session::decode(PacketReader& packet) {
	return RTMFP::Decode(_aesDecrypt,packet);
}

inline Poco::UInt32 Session::id() const {
	return _id;
}

inline Poco::UInt32 Session::farId() const {
	return _farId;
}


} // namespace Cumulus
