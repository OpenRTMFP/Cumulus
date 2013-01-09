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

#include "TCPClient.h"
#include "SocketManager.h"
#include "Logs.h"
#include "Poco/Format.h"
#include <cstring>


using namespace std;
using namespace Poco;
using namespace Cumulus;
using namespace Poco::Net;

TCPClient::TCPClient(const StreamSocket& socket,SocketManager& manager) : _pSocket(new StreamSocket(socket)),_connected(true),_manager(manager) {
	_pSocket->setBlocking(false);
	_manager.add(*_pSocket,*this);
}

TCPClient::TCPClient(SocketManager& manager) : _connected(false),_manager(manager),_pSocket(new StreamSocket()) {
}


TCPClient::~TCPClient() {
	disconnect();
	delete _pSocket;
}

void TCPClient::error(const string& error) {
	_error = error;
	disconnect();
}

void TCPClient::onReadable(Socket& socket) {
	UInt32 available = socket.available();
	if(available==0) {
		disconnect();
		return;
	}

	UInt32 size = _recvBuffer.size();
	_recvBuffer.resize(size+available);

	int received = _pSocket->receiveBytes(&_recvBuffer[size],available);
	if(received<=0) {
		disconnect(); // Graceful disconnection
		return;
	}
	available = size+received;

	UInt32 rest = onReception(&_recvBuffer[0],available);
	if(rest>available) {
		WARN("TCPClient : onReception has returned a 'rest' value more important than the available value (%u>%u)",rest,available);
		rest=available;
	}
	if(_recvBuffer.size()>rest) {
		if(available>rest)
			_recvBuffer.erase(_recvBuffer.begin(),_recvBuffer.begin()+(available-rest));
		_recvBuffer.resize(rest);
	}
}

void TCPClient::onWritable(Socket& socket) {
	if(_sendBuffer.size()==0)
		return;
	int sent = sendIntern(&_sendBuffer[0],_sendBuffer.size());
	if(sent<_sendBuffer.size())
		_sendBuffer.erase(_sendBuffer.begin(),_sendBuffer.begin()+sent);
	else
		_sendBuffer.clear();
}

int TCPClient::sendIntern(const UInt8* data,UInt32 size) {
	try {
		return _pSocket->sendBytes(data,size);	
	} catch (...) {}
	return 0;
}

bool TCPClient::connect(const SocketAddress& address) {
	if(_connected)
		disconnect();
	_error.clear();
	try {
		_pSocket->connectNB(address);
		_connected = true;
		_manager.add(*_pSocket,*this);
	} catch(Exception& ex) {
		error(format("Impossible to connect to %s, %s",address.toString(),ex.displayText()));
	}
	return _connected;
}

void TCPClient::disconnect() {
	if(!_connected)
		return;
	_manager.remove(*_pSocket);
	try {_pSocket->shutdown();} catch(...){}
	delete _pSocket;
	_pSocket = new StreamSocket();
	_connected = false;
	_recvBuffer.clear();
	_sendBuffer.clear();
	onDisconnection();
}

bool TCPClient::send(const UInt8* data,UInt32 size) {
	if(!_connected) {
		if(!error())
			error("TCPClient not connected");
		return false;
	}
	if(size==0)
		return true;
	int sent = sendIntern(data,size);
	if(sent<size) {	
		size -= sent;
		UInt32 oldSize = _sendBuffer.size();
		_sendBuffer.resize(oldSize+size);
		memcpy(&_sendBuffer[oldSize],&data[sent],size);
	}
	return true;
}
