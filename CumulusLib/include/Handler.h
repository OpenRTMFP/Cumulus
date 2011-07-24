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

namespace Cumulus {

class Handler {
public:
	Handler();
	virtual ~Handler();

	//events
	virtual void    onStart()=0;
	virtual void    onStop()=0;

	virtual bool	onConnection(Client& client,FlowWriterFactory& flowWriterFactory)=0;
	virtual void	onFailed(const Client& client,const std::string& error)=0;
	virtual void	onDisconnection(const Client& client)=0;
	virtual bool	onMessage(const Client& client,const std::string& name,AMFReader& reader,FlowWriter& writer)=0;

	virtual void	onPublish(const Client& client,const Publication& publication)=0;
	virtual void	onUnpublish(const Client& client,const Publication& publication)=0;

	virtual void	onAudioPacket(const Client& client,const Publication& publication,Poco::UInt32 time,PacketReader& packet)=0;
	virtual void	onVideoPacket(const Client& client,const Publication& publication,Poco::UInt32 time,PacketReader& packet)=0;

	virtual void	onSubscribe(const Client& client,const Listener& listener)=0;
	virtual void	onUnsubscribe(const Client& client,const Listener& listener)=0;

	// invocations
	Group&				group(const std::vector<Poco::UInt8>& id);
	Streams				streams;

	// properties
	const Poco::UInt32	count;
	const Poco::UInt32	keepAlivePeer;
	const Poco::UInt32	keepAliveServer;
private:
	std::list<Group*>	_groups;
};


} // namespace Cumulus
