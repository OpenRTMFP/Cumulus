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
#include "Cookie.h"

namespace Cumulus {

class EdgeSession : public Session {
public:
	EdgeSession(Poco::UInt32 id,
			Poco::UInt32 farId,
			const Peer& peer,
			const Poco::UInt8* decryptKey,
			const Poco::UInt8* encryptKey,
			Poco::Net::DatagramSocket& serverSocket,
			Cookie& cookie);
	~EdgeSession();

	const Poco::UInt32			farServerId;

	void						serverPacketHandler(PacketReader& packet);
	
private:
	void						encode(PacketWriter& packet);
	void						packetHandler(PacketReader& packet);

	Poco::Net::DatagramSocket&	_serverSocket;
	Poco::Net::SocketAddress	_sender;

	Cookie*						_pCookie;
	bool						_handshaking;
	
};


} // namespace Cumulus
