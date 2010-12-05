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

#pragma once

#include "Cumulus.h"
#include "Session.h"
#include "Poco/URI.h"
#include "Poco/Net/DatagramSocket.h"
#include <openssl/dh.h>

namespace Cumulus {


class Middle : public Session {
public:
	Middle(Poco::UInt32 id,
			Poco::UInt32 farId,
			const std::string& url,
			const Poco::UInt8* decryptKey,
			const Poco::UInt8* encryptKey,
			Poco::Net::DatagramSocket& socket,
			const std::string& listenCirrusUrl);
	~Middle();

	PacketReader requestCirrusHandshake(PacketWriter request);

	
private:
	void		packetHandler(PacketReader& packet,Poco::Net::SocketAddress& sender);
	Poco::UInt8 cirrusHandshakeHandler(Poco::UInt8 type,PacketReader& response,PacketWriter request);

	AESEngine				_handshakeDecrypt;
	AESEngine				_handshakeEncrypt;
	Poco::UInt8				_handshakeBuff[MAX_SIZE_MSG];

	AESEngine*				_pMiddleAesDecrypt;
	AESEngine*				_pMiddleAesEncrypt;
	
	Poco::UInt32				_cirrusId;
	Poco::URI					_cirrusUri;
	std::string					_middleCertificat;
	DH*							_pMiddleDH;

	Poco::Net::DatagramSocket	_socketClient;
};


} // namespace Cumulus
