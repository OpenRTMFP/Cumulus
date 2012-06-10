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

#include "ServerConnection.h"
#include "Util.h"
#include "Logs.h"
#include "Poco/Format.h"

using namespace std;
using namespace Cumulus;
using namespace Poco;
using namespace Poco::Net;


ServerConnection::ServerConnection(const string& address,SocketManager& socketManager,ServerHandler& handler,ServersHandler& serversHandler) : address(address),_size(0),_handler(handler),TCPClient(socketManager),_connected(false),_serversHandler(serversHandler) {
}

ServerConnection::ServerConnection(const StreamSocket& socket,SocketManager& socketManager,ServerHandler& handler,ServersHandler& serversHandler) : address(socket.peerAddress().toString()),_size(0),_handler(handler),TCPClient(socket,socketManager),_connected(false),_serversHandler(serversHandler) {
	sendPublicAddress();
}


ServerConnection::~ServerConnection() {
	
}

void ServerConnection::sendPublicAddress() {
	ServerMessage message;
	if(_handler.publicAddress().empty()) {
		message.write8(0);
		message.write16(_handler.port());
	} else {
		message.write8(1);
		message << _handler.publicAddress();
	}
	send("",message);
}

void ServerConnection::connect() {
	if(connected())
		return;
	INFO("Attempt to join %s server",address.c_str())
	TCPClient::connect(SocketAddress(address));
	sendPublicAddress();
}

void ServerConnection::send(const string& handler,ServerMessage& message) {
	string handlerName(handler);
	if(handlerName.size()>255) {
		handlerName.resize(255);
		WARN("The server handler '%s' truncated for 255 char (maximum acceptable size)",handlerName.c_str())
	}

	// Search handler!
	UInt32 handlerRef=0;
	bool   writeRef = false;
	if(!handlerName.empty()) {
		map<string,UInt32>::iterator it = _sendingRefs.lower_bound(handlerName);
		if(it!=_sendingRefs.end() && it->first==handlerName) {
			handlerRef = it->second;
			handlerName.clear();
			writeRef = true;
		} else {
			if(it!=_sendingRefs.begin())
				--it;
			handlerRef = _sendingRefs.size()+1;
			_sendingRefs.insert(it,pair<string,UInt32>(handlerName,handlerRef));
		}
	}

	UInt16 shift = handlerName.empty() ? Util::Get7BitValueSize(handlerRef) : handlerName.size();
	shift = 300-(shift+5);

	BinaryStream& stream = message._stream;

	stream.resetReading(shift);
	UInt32 size = stream.size();
	stream.resetWriting(shift);
	message.write32(size-4);
	message.writeString8(handlerName);
	if(writeRef)
		message.write7BitEncoded(handlerRef);
	else if(handlerName.empty())
		message.write8(0);
	stream.resetWriting(size+shift);

	Logs::Dump(stream.data()+4,stream.size()-4,format("To %s server",address).c_str(),true);
	TCPClient::send(stream.data(),stream.size());
}


UInt32 ServerConnection::onReception(const UInt8* data,UInt32 size){
	if(_size==0 && size<4)
		return size;

	PacketReader reader(data,size);
	if(_size==0)
		_size = reader.read32();
	if(reader.available()<_size)
		return reader.available();

	size = reader.available()-_size;
	reader.shrink(_size);
	
	Logs::Dump(reader,format("From %s server",address).c_str(),true);

	string handler;
	UInt8 handlerSize = reader.read8();
	if(handlerSize) {
		reader.readRaw(handlerSize,handler);
		_receivingRefs[_receivingRefs.size()+1] = handler;
	} else {
		UInt32 ref = reader.read7BitEncoded();
		if(ref>0) {
			map<UInt32,string>::const_iterator it = _receivingRefs.find(ref);
			if(it==_receivingRefs.end())
				ERROR("Impossible to find the %u handler reference for the server %s",ref,peerAddress().toString().c_str())
			else
				handler.assign(it->second);
		}
	}

	_size=0;
	if(handler.empty()) {
		if(reader.read8())
			reader >> ((string&)publicAddress);
		else
			(string&)publicAddress = format("%s:%hu",peerAddress().host().toString(),reader.read16());
		if(!_connected) {
			_connected=true;
			_serversHandler.connection(*this);
			_handler.connection(*this);
		}
		return size;
	}
	_handler.message(*this,handler,reader);

	return size;
}

void ServerConnection::onDisconnection(){
	_sendingRefs.clear();
	_receivingRefs.clear();
	if(_connected) {
		_connected=false;
		_handler.disconnection(*this,error());
		_serversHandler.disconnection(*this);
	}
}
