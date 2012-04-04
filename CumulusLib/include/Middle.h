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
#include "ServerSession.h"
#include "Sessions.h"
#include "Target.h"
#include "Poco/URI.h"
#include <openssl/dh.h>

namespace Cumulus {


class Middle : public ServerSession, public SocketHandler {
public:
	Middle(SendingEngine& sendingEngine,
			Poco::UInt32 id,
			Poco::UInt32 farId,
			const Peer& peer,
			const Poco::UInt8* decryptKey,
			const Poco::UInt8* encryptKey,
			Handler& handler,
			const Sessions&	sessions,
			Target& target);
	~Middle();

	const Peer&			middlePeer();

	void				targetPacketHandler(PacketReader& packet);

	PacketWriter&		handshaker();
	void				sendHandshakeToTarget(Poco::UInt8 type);

	void				manage();
	
private:
	AESEngine	decoder();
	AESEngine	encoder();

	PacketWriter&		writer();

	void				targetHandshakeHandler(Poco::UInt8 type,PacketReader& packet);
	PacketWriter&		requester();
	void				packetHandler(PacketReader& packet);
	void				sendToTarget();

	void				failSignal();

	void				onReadable(const Poco::Net::Socket& socket);
	void				onError(const Poco::Net::Socket& socket,const std::string& error);

	AESEngine*				_pMiddleAesDecrypt;
	AESEngine*				_pMiddleAesEncrypt;
	
	std::string					_queryUrl;
	Poco::UInt32				_middleId;
	Peer						_middlePeer;
	Poco::UInt8					_middleCertificat[76];
	DH*							_pMiddleDH;
	Target&						_target;
	std::vector<Poco::UInt8>	_targetNonce;
	bool						_isPeer;
	Poco::UInt8					_sharedSecret[KEY_SIZE];

	Poco::Net::DatagramSocket	_socket;
	Poco::UInt8					_buffer[PACKETRECV_SIZE];

	bool						_firstResponse;

	const Sessions&				_sessions;
};

inline void Middle::manage() {
	// just to overload the ServerSession version
}


inline const Peer& Middle::middlePeer() {
	return _middlePeer;
}

inline AESEngine Middle::decoder() {
	return aesDecrypt.next(farId==0 ? AESEngine::SYMMETRIC : AESEngine::DEFAULT);
}

inline AESEngine Middle::encoder() {
	return aesEncrypt.next(farId==0 ? AESEngine::SYMMETRIC : AESEngine::DEFAULT);
}


} // namespace Cumulus
