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

#include "UDPSocket.h"
#include "Logs.h"
#include "Poco/Buffer.h"
#include "Poco/Format.h"
#include "string.h"


using namespace std;
using namespace Cumulus;
using namespace Poco;
using namespace Poco::Net;



class UDPSending : public WorkThread {
public:
	UDPSending(DatagramSocket& socket,const UInt8* buffer,UInt32 size,const SocketAddress& address) : _buffer(size),_socket(socket),_address(address){
		memcpy(_buffer.begin(),buffer,size);
	}
	virtual ~UDPSending(){}

	void run() {
		try {
			int sent = _address.host().isWildcard() ? _socket.sendBytes(_buffer.begin(),_buffer.size()) : _socket.sendTo(_buffer.begin(),_buffer.size(),_address);
			if(sent<_buffer.size())
				ERROR("UDPSending error, all data have not been sent");
		} catch(Exception& ex) {
			 ERROR("UDPSending error, %s",ex.displayText().c_str());
		} catch(exception& ex) {
			 ERROR("UDPSending error, %s",ex.what());
		} catch(...) {
			 ERROR("UDPSending unknown error");
		}
	}

private:
	SocketAddress	_address;
	DatagramSocket	_socket;
	Buffer<UInt8>	_buffer;		
};



UDPSocket::UDPSocket(PoolThreads& poolThreads,SocketManager& manager,bool allowBroadcast) : _bound(false),_connected(false),_manager(manager),_recvBuffer(8192),_pSendingThread(NULL),_poolThreads(poolThreads) {
	_socket.setBroadcast(allowBroadcast);
	
}

UDPSocket::~UDPSocket() {
	close();
}

void UDPSocket::onReadable(Socket& socket) {
	UInt32 available = socket.available();
	if(available==0)
		return;

	if(available>_recvBuffer.size())
		_recvBuffer.resize(available);

	SocketAddress address;
	onReception(&_recvBuffer[0],_socket.receiveFrom(&_recvBuffer[0],available,address),address);
}

void UDPSocket::close() {
	_error.clear();
	_manager.remove(_socket);
	_socket.close();
	_connected = false;
	_bound = false;
}

bool UDPSocket::bind(const Poco::Net::SocketAddress & address) {
	_bound = false;
	_error.clear();
	if(_connected) {
		_error = "Impossible to bind a connected UDPSocket, close the socket before";
		return false;
	}
	try {
		_socket.bind(address);
		_manager.add(_socket,*this);
		_bound = true;
	} catch(Exception& ex) {
		_error = format("Impossible to bind to %s, %s",address.toString(),ex.displayText());
	}
	return _bound;
}

void UDPSocket::connect(const SocketAddress& address) {
	_error.clear();
	_connected = false;
	if(_bound) {
		_error = "Impossible to connect a bound UDPSocket, close the socket before";
		return;
	}
	try {
		_socket.connect(address);
		_connected = true;
	} catch(Exception& ex) {
		_error = format("Impossible to connect to %s, %s",address.toString(),ex.displayText());
	}
}

void UDPSocket::send(const UInt8* data,UInt32 size) {
	_error.clear();
	if(!_connected) {
		_error = "Sending without recipient on a UDPSocket not connected";
		return;
	}
	SocketAddress nullAddress;
	send(data,size,nullAddress);
}

void UDPSocket::send(const UInt8* data,UInt32 size,const SocketAddress& address) {
	_error.clear();
	if(size==0)
		return;
	_pSendingThread = _poolThreads.enqueue(AutoPtr<UDPSending>(new UDPSending(_socket,data,size,address)),_pSendingThread);
}
