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
#include "Client.h"
#include "Address.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/DatagramSocket.h"

namespace Cumulus {

class Group;
class Handler;
class Publication;
class Listener;
class Peer : public Client {
public:
	Peer(Handler& handler);
	virtual ~Peer();

	Poco::Net::SocketAddress		address;
	std::list<Address>				addresses;

	const bool						connected;
	bool							closed();

	void setFlowWriter(FlowWriter* pWriter);

	void unsubscribeGroups();
	void joinGroup(Group& group);
	void joinGroup(const Poco::UInt8* id);
	void unjoinGroup(Group& group);


// events
	bool onConnection(AMFReader& parameters,AMFObjectWriter& response);
	void onFailed(const std::string& error);
	void onDisconnection();
	bool onMessage(const std::string& name,AMFReader& reader);

	bool onPublish(const Publication& publication,std::string& error);
	void onUnpublish(const Publication& publication);

	void onDataPacket(const Publication& publication,const std::string& name,PacketReader& packet);
	void onAudioPacket(const Publication& publication,Poco::UInt32 time,PacketReader& packet);
	void onVideoPacket(const Publication& publication,Poco::UInt32 time,PacketReader& packet);

	bool onSubscribe(const Listener& listener,std::string& error);
	void onUnsubscribe(const Listener& listener);

private:
	void onJoinGroup(Group& group);
	void onUnjoinGroup(Group& group);

	Handler&						_handler;
	std::map<Group*,Poco::UInt32>	_groups;
};

inline bool Peer::closed(){
	return _closed;
}

inline void Peer::setFlowWriter(FlowWriter* pWriter){
	_pFlowWriter = pWriter;
}

} // namespace Cumulus
