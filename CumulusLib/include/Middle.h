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
#include "Session.h"
#include "Cirrus.h"
#include "Poco/URI.h"
#include <openssl/dh.h>

namespace Cumulus {


class Middle : public Session {
public:
	Middle(Poco::UInt32 id,
			Poco::UInt32 farId,
			const Peer& peer,
			const std::string& url,
			const Poco::UInt8* decryptKey,
			const Poco::UInt8* encryptKey,
			Poco::Net::DatagramSocket& socket,
			ServerData& data,
			Cirrus& cirrus);
	~Middle();

	const Peer&			middlePeer();

	void				cirrusHandshakeHandler(Poco::UInt8 type,PacketReader& packet);
	void				cirrusPacketHandler(PacketReader& packet);

	const Peer*			pPeerWanted;

	void				sendHandshakeToCirrus(Poco::UInt8 type,PacketWriter& request);
private:
	bool				manage();
	void				sendToCirrus(Poco::UInt32 id,PacketWriter& request);
	
	
	void		packetHandler(PacketReader& packet);
	Poco::UInt8 cirrusHandshakeHandler(Poco::UInt8 type,PacketReader& response,PacketWriter request);

	AESEngine*				_pMiddleAesDecrypt;
	AESEngine*				_pMiddleAesEncrypt;
	
	Poco::UInt32				_middleId;
	Peer						_middlePeer;
	std::string					_middleCertificat;
	DH*							_pMiddleDH;
	Cirrus&						_cirrus;

	Poco::Net::DatagramSocket	_socket;
	Poco::Timespan				_span;
	Poco::UInt8					_buffer[MAX_SIZE_MSG];

};


inline const Peer& Middle::middlePeer() {
	return _middlePeer;
}


} // namespace Cumulus
