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

#include "Server.h"
#include "Util.h"
#include "Poco/Util/ServerApplication.h"

using namespace std;
using namespace Poco;
using namespace Poco::Util;
using namespace Poco::Net;
using namespace Cumulus;


Server::Server(Auth& auth,ApplicationKiller& applicationKiller) : _auth(auth),_applicationKiller(applicationKiller),RTMFPServer(){
}


Server::~Server() {

}

void Server::manage() {
	// TODO LUA
	RTMFPServer::manage();
}

void Server::onStart() {

}
void Server::onStop() {
	_applicationKiller.kill();
}

//// CLIENT_HANDLER /////
bool Server::onConnection(Client& client,FlowWriterFactory& flowWriterFactory) {
	// Here you can read custom client http parameters in reading "client.parameters".
	// Also you can send custom data for the client in writing in "client.data",
	// on flash side you could read that on "data" property from NetStatusEvent::NET_STATUS event of NetConnection object
	bool result = _auth.check(client);

	// Status page
	if(result && client.path == "/status")
		_status.insert(pair<const Client* const,StatusWriter*>(&client,&flowWriterFactory.newFlowWriter<StatusWriter>())).first->second->init(*this);

	return result;
}
void Server::onFailed(const Client& client,const string& msg) {
	WARN("Client failed : %s",msg.c_str());
}
void Server::onDisconnection(const Client& client) {
	// Status page
	_status.erase(&client);
}

bool Server::onMessage(const Client& client,const string& name,AMFReader& reader,FlowWriter& writer) {
	DEBUG("onMessage %s",name.c_str());

	// Status page
	if(name=="addList") {
		map<const Client* const,StatusWriter*>::const_iterator it = _status.find(&client);
		if(it!=_status.end()) {
			it->second->idPublisherWithListeners = (UInt32)reader.readNumber();
			return true;
		}
	} else if(name=="remList") {
		map<const Client* const,StatusWriter*>::const_iterator it = _status.find(&client);
		if(it!=_status.end()) {
			it->second->idPublisherWithListeners = 0;
			return true;
		}
	}
	return false;
}

//// PUBLICATION_HANDLER /////
void Server::onPublish(const Client& client,const Publication& publication) {
	// Status
	map<const Client* const,StatusWriter*>::const_iterator it2;
	for(it2=_status.begin();it2!=_status.end();++it2)
		it2->second->addPublication(publication);
}

void Server::onUnpublish(const Client& client,const Publication& publication) {
	// Status page
	map<const Client* const,StatusWriter*>::const_iterator it2;
	for(it2=_status.begin();it2!=_status.end();++it2)
		it2->second->removePublication(publication);
}

void Server::onSubscribe(const Client& client,const Listener& listener) {
}

void Server::onUnsubscribe(const Client& client,const Listener& listener) {
}

void Server::onAudioPacket(const Client& client,const Publication& publication,UInt32 time,PacketReader& packet) {
	// Status page
	if(publication.audioQOS().lostRate==0)
		return;
	map<const Client* const,StatusWriter*>::const_iterator it2;
	for(it2=_status.begin();it2!=_status.end();++it2)
		it2->second->audioLostRate(publication);
}

void Server::onVideoPacket(const Client& client,const Publication& publication,UInt32 time,PacketReader& packet) {
	// Status page
	if(publication.videoQOS().lostRate==0)
		return;
	map<const Client* const,StatusWriter*>::const_iterator it2;
	for(it2=_status.begin();it2!=_status.end();++it2)
		it2->second->videoLostRate(publication);
}
