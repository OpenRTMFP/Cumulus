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
#include "Poco/Net/DatagramSocket.h"

namespace Cumulus {

class Session
{
public:

	Session(Poco::UInt32 id,
			Poco::UInt32 farId,
			const Peer& peer,
			const std::string& url,
			const Poco::UInt8* decryptKey,
			const Poco::UInt8* encryptKey,
			Poco::Net::DatagramSocket& socket,
			ServerData& data);

	virtual ~Session();

	virtual void	packetHandler(PacketReader& packet);

	Poco::UInt32	id() const;
	Poco::UInt32	farId() const;
	const Peer& 	peer() const;
	virtual bool	manage();

	void	p2pHandshake(const Peer& peer);

	bool	decode(PacketReader& packet);

protected:
	

	void			send(Poco::UInt8 marker,PacketWriter packet,bool forceSymetricEncrypt=false);

	Poco::UInt32	_farId; // Protected for Middle session

private:
	
	
	Flow*				createFlow(Poco::UInt8 id);
	Flow&				flow(Poco::UInt8 id);
	

	void				keepaliveHandler();

	Poco::Timestamp				_timeUpdate;
	ServerData&					_data;

	std::map<Poco::UInt8,Flow*> _flows;	

	std::string					_url;
	Poco::UInt32				_id;

	Poco::Net::DatagramSocket&	_socket;
	AESEngine					_aesDecrypt;
	AESEngine					_aesEncrypt;

	Peer						_peer;
};

inline bool Session::decode(PacketReader& packet) {
	return RTMFP::Decode(_aesDecrypt,packet);
}

inline const Peer& Session::peer() const {
	return _peer;
}

inline Poco::UInt32 Session::id() const {
	return _id;
}

inline Poco::UInt32 Session::farId() const {
	return _farId;
}


} // namespace Cumulus
