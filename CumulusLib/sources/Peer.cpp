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

#include "Peer.h"
#include "Group.h"
#include "Handler.h"
#include "Util.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

Peer::Peer(Handler& handler):_handler(handler),connected(false){
}

Peer::~Peer() {
	unsubscribeGroups();
}

void Peer::joinGroup(const UInt8* id) {
	// create group is need
	Entities<Group>::Map::iterator it = _handler._groups.lower_bound(id);
	Group* pGroup = NULL;
	if(it==_handler._groups.end() || it->first!=id) {
		if(it!=_handler._groups.begin())
			--it;
		pGroup = new Group(id);
		_handler._groups.insert(it,pair<const UInt8*,Group*>(pGroup->id,pGroup));
	} else
		pGroup = it->second;
	joinGroup(*pGroup);
}

void Peer::joinGroup(Group& group) {
	map<Group*,UInt32>::iterator it = _groups.lower_bound(&group);
	if(it!=_groups.end() && it->first==&group)
		return;

	if(it!=_groups.begin())
		--it;

	// TODO What do you do if no more index available? make a reset of the map?
	UInt32 index = group._peers.size()==0 ? 0 : (group._peers.rbegin()->first+1);
	group._peers[index] = this;
	_groups.insert(it,pair<Group*,UInt32>(&group,index));
	onJoinGroup(group);
}


void Peer::unjoinGroup(Group& group) {
	map<Group*,UInt32>::iterator it = _groups.lower_bound(&group);
	if(it==_groups.end() || it->first==&group)
		return;

	group._peers.erase(it->second);
	_groups.erase(it);
	if(group.size()==0) {
		_handler._groups.erase(group.id);
		onUnjoinGroup(group);
		delete &group;
	}
}

void Peer::unsubscribeGroups() {
	map<Group*,UInt32>::const_iterator it=_groups.begin();
	for(it=_groups.begin();it!=_groups.end();++it) {
		Group& group = *it->first;
		group._peers.erase(it->second);
		if(group.size()==0) {
			_handler._groups.erase(group.id);
			onUnjoinGroup(group);
			delete &group;
		}
	}
	_groups.clear();
}

/// EVENTS ///

bool Peer::onConnection(AMFReader& parameters,AMFObjectWriter& response) {
	if(!connected) {
		(bool&)connected = _handler.onConnection(*this,parameters,response);
		if(connected) {
			if(!_handler._clients.insert(pair<const UInt8*,Client*>(id,this)).second)
				ERROR("Client %s seems already connected!",Util::FormatHex(id,ID_SIZE).c_str())
			else {
				Edge* pEdge = _handler.edges(address);
				if(pEdge)
					++(UInt32&)pEdge->count;
			}
		}
	} else
		ERROR("Client %s seems already connected!",Util::FormatHex(id,ID_SIZE).c_str())
	return connected;
}

void Peer::onFailed(const string& error) {
	if(connected)
		_handler.onFailed(*this,error);
}

void Peer::onDisconnection() {
	if(connected) {
		(bool&)connected = false;
		if(_handler._clients.erase(id)==0)
			ERROR("Client %s seems already disconnected!",Util::FormatHex(id,ID_SIZE).c_str())
		else {
			Edge* pEdge = _handler.edges(address);
			if(pEdge)
				--(UInt32&)pEdge->count;
		}
		_handler.onDisconnection(*this);
	}
}

bool Peer::onMessage(const string& name,AMFReader& reader) {
	if(connected)
		return _handler.onMessage(*this,name,reader);
	WARN("RPC client before connection")
	writer().writeErrorResponse("Call.Failed","Client must be connected before remote procedure calling");
	return true;
}

void Peer::onJoinGroup(Group& group) {
	if(connected) {
		_handler.onJoinGroup(*this,group);
		return;
	}
	WARN("Group joining client before connection")
}

void Peer::onUnjoinGroup(Group& group) {
	if(connected) {
		_handler.onUnjoinGroup(*this,group);
		return;
	}
	WARN("Group unjoining client before connection")
}

bool Peer::onPublish(const Publication& publication,string& error) {
	if(connected)
		return _handler.onPublish(*this,publication,error);
	WARN("Publication client before connection")
	error = "Client must be connected before publication";
	return false;
}

void Peer::onUnpublish(const Publication& publication) {
	if(connected) {
		_handler.onUnpublish(*this,publication);
		return;
	}
	WARN("Unpublication client before connection")
}

bool Peer::onSubscribe(const Listener& listener,string& error) {
	if(connected)
		return _handler.onSubscribe(*this,listener,error);
	WARN("Subscription client before connection")
	error = "Client must be connected before subscription";
	return false;
}

void Peer::onUnsubscribe(const Listener& listener) {
	if(connected) {
		_handler.onUnsubscribe(*this,listener);
		return;
	}
	WARN("Unsubscription client before connection")
}

void Peer::onDataPacket(const Publication& publication,const string& name,PacketReader& packet) {
	if(connected) {
		_handler.onDataPacket(*this,publication,name,packet);
		return;
	}
	WARN("DataPacket client before connection")
}

void Peer::onAudioPacket(const Publication& publication,Poco::UInt32 time,PacketReader& packet) {
	if(connected) {
		_handler.onAudioPacket(*this,publication,time,packet);
		return;
	}
	WARN("AudioPacket client before connection")
}

void Peer::onVideoPacket(const Publication& publication,Poco::UInt32 time,PacketReader& packet) {
	if(connected) {
		_handler.onVideoPacket(*this,publication,time,packet);
		return;
	}
	WARN("VideoPacket client before connection")
}





} // namespace Cumulus
