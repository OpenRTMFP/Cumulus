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
#include "Poco/Net/StreamSocket.h"


class TCPClient : private Cumulus::SocketHandler {
public:
	TCPClient(const Poco::Net::StreamSocket& socket,Cumulus::SocketManager& manager);
	TCPClient(Cumulus::SocketManager& manager);
	virtual ~TCPClient();

	bool					 connect(const std::string& host,Poco::UInt16 port);
	bool					 connected();
	void					 disconnect();

	bool					 send(const Poco::UInt8* data,Poco::UInt32 size);

	Poco::Net::SocketAddress address();
	Poco::Net::SocketAddress peerAddress();

	const char*				error();

private:
	virtual Poco::UInt32		onReception(const Poco::UInt8* data,Poco::UInt32 size)=0;
	virtual void				onDisconnection(){}

	bool						haveToWrite(const Poco::Net::Socket& socket);
	void						onReadable(Poco::Net::Socket& socket);
	void						onWritable(Poco::Net::Socket& socket);
	void						onError(const Poco::Net::Socket& socket,const std::string& error);

	void						error(const std::string& error);

	int							sendIntern(const Poco::UInt8* data,Poco::UInt32 size);

	std::string					_error;
	Poco::Net::StreamSocket		_socket;
	std::vector<Poco::UInt8>	_recvBuffer;
	std::vector<Poco::UInt8>	_sendBuffer;
	bool						_connected;
	Cumulus::SocketManager&		_manager;
};

inline Poco::Net::SocketAddress	TCPClient::address() {
	return _connected ? _socket.address() : Poco::Net::SocketAddress();
}

inline Poco::Net::SocketAddress	TCPClient::peerAddress() {
	return _connected ? _socket.peerAddress() : Poco::Net::SocketAddress();
}

inline void TCPClient::onError(const Poco::Net::Socket& socket,const std::string& error) {
	this->error("TCPClient error, " + error);
}

inline bool TCPClient::haveToWrite(const Poco::Net::Socket& socket) {
	return !_sendBuffer.empty();
}

inline bool TCPClient::connected() {
	return _connected;
}

inline const char* TCPClient::error() {
	return _error.empty() ? NULL : _error.c_str();
}
