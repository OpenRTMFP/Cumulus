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

#include "Servers.h"
#include "Logs.h"
#include "Poco/StringTokenizer.h"

using namespace std;
using namespace Cumulus;
using namespace Poco;
using namespace Poco::Net;


Servers::Servers(UInt16 port,ServerHandler& handler,SocketManager& manager,const string& addresses) : TCPServer(manager),_port(port),_handler(handler),_manageTimes(1) {
	if(port>0)
		NOTE("Servers incoming connection enabled on port %hu",port)
	else if(!_destinators.empty())
		NOTE("Servers incoming connection disabled (servers.port==0)")

	StringTokenizer tokens(addresses,",",StringTokenizer::TOK_IGNORE_EMPTY | StringTokenizer::TOK_TRIM);
	StringTokenizer::Iterator it;
	for(it=tokens.begin();it!=tokens.end();++it)
		_destinators.insert(new ServerConnection(*it,manager,_handler,*this));
}

Servers::~Servers() {
	stop();
	set<ServerConnection*>::const_iterator it;
	for(it=_destinators.begin();it!=_destinators.end();++it)
		delete (*it);
}

ServerConnection* Servers::find(const string& address) {
	Iterator it = _connections.find(address);
	return it==end() ? NULL : it->second;
}

void Servers::manage() {
	if(_destinators.empty() || (--_manageTimes)!=0)
		return;
	_manageTimes = 5; // every 10 sec
	set<ServerConnection*>::const_iterator it;
	for(it=_destinators.begin();it!=_destinators.end();++it)
		(*it)->connect();
}

void Servers::start() {
	if(_port>0)
		TCPServer::start(_port);
}


void Servers::stop() {
	TCPServer::stop();
	_connections.clear();
	set<ServerConnection*>::iterator it;
	for(it=_clients.begin();it!=_clients.end();++it)
		delete (*it);
	_clients.clear();
}


void Servers::broadcast(const string& handler,ServerMessage& message) {
	Servers::Iterator it;
	for(it=begin();it!=end();++it)
		it->second->send(handler,message);
}

void Servers::connection(ServerConnection& server) {
	_connections[server.publicAddress] = &server;
	NOTE("Connection etablished with %s server ",server.publicAddress.c_str())
}

void Servers::disconnection(ServerConnection& server) {
	_connections.erase(server.publicAddress);
	_clients.erase(&server);
	NOTE("Disconnection from %s server ",server.publicAddress.c_str())
	if(_destinators.find(&server)==_destinators.end())
		delete &server;
}
