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
#include "EdgeSession.h"

namespace Cumulus {


class P2PHandshakerAddress : public Poco::Net::SocketAddress {
public:
	P2PHandshakerAddress() {}
	P2PHandshakerAddress& operator=(const Poco::Net::SocketAddress& address){
		Poco::Net::SocketAddress::operator=(address);
		_time.update();
		return *this;
	}
	virtual ~P2PHandshakerAddress() {}
	bool obsolete(){return _time.isElapsed(120000000);}
private:
	Poco::Timestamp _time;
};


class ServerConnection : public ServerSession {
public:
	ServerConnection(Handler& handler,Session& handshake);
	~ServerConnection();

	bool	connected();
	void	connect(const Poco::Net::SocketAddress& publicAddress);
	void	disconnect();

	void	manage();
	void    clear();

	void	createSession(EdgeSession& session,const std::string& url);
	void	sendP2PHandshake(const std::string& tag,const Poco::Net::SocketAddress& address,const Poco::UInt8* peerIdWanted);

private:
	void	flush();
	
	void	packetHandler(PacketReader& packet);

	Session&										_handshake;
	bool											_connected;
	std::map<std::string,P2PHandshakerAddress>		_p2pHandshakers;

};

inline void ServerConnection::clear() {
	_p2pHandshakers.clear();
}

inline void ServerConnection::flush() {
	ServerSession::flush(0x0b,false);
}

inline bool ServerConnection::connected() {
	return _connected;
}


} // namespace Cumulus
