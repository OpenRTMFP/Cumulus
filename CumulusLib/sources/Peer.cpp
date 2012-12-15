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

class Member {
public:
	Member(Poco::UInt32	index,FlowWriter* pWriter) : index(index),pWriter(pWriter){}
	const Poco::UInt32	index;
	FlowWriter*			pWriter;
};

Peer::Peer(Handler& handler):_handler(handler),connected(false),addresses(1) {
}

Peer::~Peer() {
	unsubscribeGroups();
}

Group& Peer::joinGroup(const UInt8* id,FlowWriter* pWriter) {
	// create group if need
	Entities<Group>::Map::iterator it = _handler._groups.lower_bound(id);
	Group* pGroup = NULL;
	if(it==_handler._groups.end() || it->first!=id) {
		if(it!=_handler._groups.begin())
			--it;
		pGroup = new Group(id);
		_handler._groups.insert(it,pair<const UInt8*,Group*>(pGroup->id,pGroup));
	} else
		pGroup = it->second;
	joinGroup(*pGroup,pWriter);
	return *pGroup;
}

bool Peer::writeId(Group& group,Peer& peer,FlowWriter* pWriter) {
	if(pWriter) {
		BinaryWriter& response(pWriter->writeRawMessage(true));
		response.write8(0x0b); // unknown
		response.writeRaw(peer.id,ID_SIZE);
	} else {
		map<Group*,Member*>::const_iterator it = peer._groups.find(&group);
		if(it==peer._groups.end()) {
			CRITIC("A peer in a group without have its _groups collection associated")
			return false;
		}
		if(!it->second->pWriter)
			return false;
		BinaryWriter& response(it->second->pWriter->writeRawMessage(true));
		response.write8(0x0b); // unknown
		response.writeRaw(id,ID_SIZE);
	}
	return true;
}

void Peer::joinGroup(Group& group,FlowWriter* pWriter) {
	UInt16 count=5;
	Group::Iterator it0=group.end();
	while(group.size()>0) {
		if(group.begin()==it0)
			break;
		--it0;
		Client& client = **it0;
		if(client==this->id)
			continue;
		if(!writeId(group,(Peer&)client,pWriter))
			continue;
		if(--count==0)
			break;
	}
	// + 1 random!
	if(it0!=group.end()) {
		UInt32 distance = Group::Distance(group.begin(),it0);
		if(distance>0) {
			it0 = group.begin();
			Group::Advance(it0,rand() % distance);
			writeId(group,(Peer&)**it0,pWriter);
		}
	}

	map<Group*,Member*>::iterator it = _groups.lower_bound(&group);
	if(it!=_groups.end() && it->first==&group)
		return;

	if(it!=_groups.begin())
		--it;

	UInt32 index = 0;
	if(!group._peers.empty()){
		index = group._peers.rbegin()->first+1;
		if(index<group._peers.rbegin()->first) {
			// max index reached, rewritten index!
			index=0;
			map<UInt32,Peer*>::iterator it1;
			for(it1=group._peers.begin();it1!=group._peers.end();++it1)
				(UInt32&)it1->first = index++;
		}
	}
	group._peers[index] = this;
	_groups.insert(it,pair<Group*,Member*>(&group,new Member(index,pWriter)));
	onJoinGroup(group);
}


void Peer::unjoinGroup(Group& group) {
	map<Group*,Member*>::iterator it = _groups.lower_bound(&group);
	if(it==_groups.end() || it->first!=&group)
		return;
	onUnjoinGroup(it);
}

void Peer::unsubscribeGroups() {
	map<Group*,Member*>::iterator it=_groups.begin();
	while(it!=_groups.end())
		onUnjoinGroup(it++);
}

/// EVENTS ///
void Peer::onHandshake(Poco::UInt32 attempts,std::set<std::string>& addresses) {
	_handler.onHandshake(address,path,properties,attempts,addresses);
}

bool Peer::onConnection(AMFReader& parameters,AMFObjectWriter& response) {
	if(!connected) {
		(bool&)connected = _handler.onConnection(*this,parameters,response);
		if(connected) {
			if(!_handler._clients.insert(pair<const UInt8*,Client*>(id,this)).second)
				ERROR("Client %s seems already connected!",Util::FormatHex(id,ID_SIZE).c_str())
		}
	} else
		ERROR("Client %s seems already connected!",Util::FormatHex(id,ID_SIZE).c_str())
	return connected;
}

void Peer::onFailed(const string& error) {
	if(connected)
		_handler.onFailed(*this,error);
	else
		WARN("Client failed : %s",error.c_str());
}

void Peer::onDisconnection() {
	if(connected) {
		(bool&)connected = false;
		if(_handler._clients.erase(id)==0)
			ERROR("Client %s seems already disconnected!",Util::FormatHex(id,ID_SIZE).c_str())
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

void Peer::onManage() {
	if(connected) {
		_handler.onManage(*this);
		return;
	}
}

void Peer::onJoinGroup(Group& group) {
	if(!connected)
		return;
	_handler.onJoinGroup(*this,group);
}

void Peer::onUnjoinGroup(map<Group*,Member*>::iterator it) {
	Group& group = *it->first;
	map<UInt32,Peer*>::iterator itPeer = group._peers.find(it->second->index);
	group._peers.erase(itPeer++);
	delete it->second;
	_groups.erase(it);

	if(connected)
		_handler.onUnjoinGroup(*this,group);

	if(group.size()==0) {
		_handler._groups.erase(group.id);
		delete &group;
	} else if(itPeer!=group._peers.end()) {
		// if a peer disconnects of one group, give to its following peer the 6th preceding peer
		Peer& followingPeer = *itPeer->second;
		UInt8 count=6;
		while(--count!=0 && itPeer!=group._peers.begin())
			--itPeer;
		if(count==0)
			itPeer->second->writeId(group,followingPeer,NULL);
	}
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
