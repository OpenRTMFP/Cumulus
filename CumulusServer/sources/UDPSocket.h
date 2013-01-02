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


#include "SocketManager.h"
#include "PoolThreads.h"
#include "Poco/Net/DatagramSocket.h"


class UDPSocket : private Cumulus::SocketHandler {
public:
	UDPSocket(Cumulus::PoolThreads& poolThreads,Cumulus::SocketManager& manager,bool allowBroadcast=false);
	virtual ~UDPSocket();

	bool					 bind(const Poco::Net::SocketAddress & address);
	void					 connect(const Poco::Net::SocketAddress& address);
	void					 close();

	void					 send(const Poco::UInt8* data,Poco::UInt32 size);
	void					 send(const Poco::UInt8* data,Poco::UInt32 size,const Poco::Net::SocketAddress& address);

	Poco::Net::SocketAddress address();
	Poco::Net::SocketAddress peerAddress();

	const char*				 error();

private:
	virtual void			onReception(const Poco::UInt8* data,Poco::UInt32 size,const Poco::Net::SocketAddress& address)=0;

	void					onReadable(Poco::Net::Socket& socket);
	void					onError(const Poco::Net::Socket& socket,const std::string& error);

	std::string					_error;
	Poco::Net::DatagramSocket*	_pSocket;
	std::vector<Poco::UInt8>	_recvBuffer;
	bool						_connected;
	bool						_bound;
	Cumulus::SocketManager&		_manager;
	Cumulus::PoolThreads&		_poolThreads;
	Cumulus::PoolThread*		_pSendingThread;
};

inline Poco::Net::SocketAddress	UDPSocket::address() {
	return _connected ? _pSocket->address() : Poco::Net::SocketAddress();
}

inline Poco::Net::SocketAddress	UDPSocket::peerAddress() {
	return _connected ? _pSocket->peerAddress() : Poco::Net::SocketAddress();
}

inline void UDPSocket::onError(const Poco::Net::Socket& socket,const std::string& error) {
	_error = "UDPSocket error, " + error;
}

inline const char* UDPSocket::error() {
	return _error.empty() ? NULL : _error.c_str();
}
