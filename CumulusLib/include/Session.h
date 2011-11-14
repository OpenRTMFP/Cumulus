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
#include "Poco/Net/DatagramSocket.h"

namespace Cumulus {

class Session {
public:

	Session(Poco::UInt32 id,
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

	bool				middleDump;

	virtual void		manage(){}

	void				setEndPoint(Poco::Net::DatagramSocket& socket,const Poco::Net::SocketAddress& address);
	void				receive(PacketReader& packet);
	void				send(PacketWriter& packet);
	
protected:
	void				send(PacketWriter& packet,Poco::UInt32 farId,Poco::Net::DatagramSocket& socket,const Poco::Net::SocketAddress& receiver);

	virtual bool		decode(PacketReader& packet);
	virtual void		encode(PacketWriter& packet);

	AESEngine			aesDecrypt;
	AESEngine			aesEncrypt;

private:
	virtual void	packetHandler(PacketReader& packet)=0;
	
	Poco::Net::DatagramSocket*	_pSocket;
};

inline bool Session::decode(PacketReader& packet) {
	return RTMFP::Decode(aesDecrypt,packet);
}

inline void Session::encode(PacketWriter& packet) {
	RTMFP::Encode(aesEncrypt,packet);
}


} // namespace Cumulus
