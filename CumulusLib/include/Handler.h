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
#include "Group.h"
#include "Streams.h"
#include "Edges.h"

namespace Cumulus {

class Handler {
	friend class Handshake; // Handshake manage _edges list!
	friend class Middle; // Middle access _groups list!
	friend class ServerSession; // ServerSession manage _clients list!
public:
	Handler();
	virtual ~Handler();

	//events
	virtual bool	onConnection(Client& client,AMFReader& parameters){return true;}
	virtual void	onFailed(const Client& client,const std::string& error){}
	virtual void	onDisconnection(const Client& client){}
	virtual bool	onMessage(Client& client,const std::string& name,AMFReader& reader){return false;}

	virtual void	onPublish(Client& client,const Publication& publication){}
	virtual void	onUnpublish(Client& client,const Publication& publication){}

	virtual void	onDataPacket(const Client& client,const Publication& publication,const std::string& name,PacketReader& packet){}
	virtual void	onAudioPacket(const Client& client,const Publication& publication,Poco::UInt32 time,PacketReader& packet){}
	virtual void	onVideoPacket(const Client& client,const Publication& publication,Poco::UInt32 time,PacketReader& packet){}

	virtual void	onSubscribe(Client& client,const Listener& listener){}
	virtual void	onUnsubscribe(Client& client,const Listener& listener){}

	// invocations
	Clients				clients;
	Group&				group(const Poco::UInt8* id);
	Streams				streams;
	Edges				edges;

	// properties
	const Poco::UInt32	udpBufferSize;
	const bool			videoSampleAccess;
	const bool			audioSampleAccess;
	const Poco::UInt32	keepAlivePeer;
	const Poco::UInt32	keepAliveServer;
	const Poco::UInt8	edgesAttemptsBeforeFallback;
private:
	std::list<Group*>											_groups;
	std::map<std::string,Edge*>									_edges;
	std::map<const Poco::UInt8*,Client*,Clients::Compare>		_clients;
};


} // namespace Cumulus
