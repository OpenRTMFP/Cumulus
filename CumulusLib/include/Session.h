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
//#include "ClientHandler.h"
#include "Poco/Timestamp.h"
#include "Poco/Net/DatagramSocket.h"

namespace Cumulus {

class Session {
public:

	Session(Poco::UInt32 id,
			Poco::UInt32 farId,
			const Peer& peer,
			const Poco::UInt8* decryptKey,
			const Poco::UInt8* encryptKey,
			Poco::Net::DatagramSocket& socket,
			ServerData& data,
			ClientHandler* pClientHandler=NULL);

	virtual ~Session();

	virtual void	packetHandler(PacketReader& packet);

	Poco::UInt32		id() const;
	Poco::UInt32		farId() const;
	const Peer& 		peer() const;
	bool				die() const;
	bool				failed() const;
	virtual void		manage();

	void	p2pHandshake(const Poco::Net::SocketAddress& address,const std::string& tag);
	void	p2pHandshake(const std::vector<Poco::Net::SocketAddress>& address,const std::string& tag);
	bool	decode(PacketReader& packet);
	
	void	fail(const std::string& msg);
protected:

	PacketWriter&	writer();
	void			send(bool symetric=false);
	void			setFailed(const std::string& msg);
	virtual void	fail();
	void			kill();

	Poco::UInt32			_farId; // Protected for Middle session
	PacketWriter			_packetOut; // Protected for Middle session
	Poco::Timestamp			_recvTimestamp; // Protected for Middle session
	Poco::UInt16			_timeSent; // Protected for Middle session

private:
	void				keepAlive();
	Flow*				createFlow(Poco::UInt8 id);
	Flow&				flow(Poco::UInt8 id,bool canCreate=false);
	
	bool						_failed;
	Poco::UInt8					_timesFailed;
	Poco::UInt8					_timesKeepalive;

	ServerData&					_data;

	std::map<Poco::UInt8,Flow*> _flows;	

	Poco::UInt32				_id;

	Poco::Net::DatagramSocket&	_socket;
	AESEngine					_aesDecrypt;
	AESEngine					_aesEncrypt;

	bool						_clientAccepted;
	ClientHandler*				_pClientHandler;
	bool						_die;
	Peer						_peer;
	Poco::UInt8					_buffer[MAX_SIZE_MSG];

	std::map<std::string,Poco::Int8> _p2pHandskakeAttempts;
};


inline void Session::fail(const std::string& msg) {
	setFailed(msg);
	fail();
}

inline bool Session::failed() const {
	return _failed;
}


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

inline bool Session::die() const {
	return _die;
}


} // namespace Cumulus
