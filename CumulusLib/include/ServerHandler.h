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
#include "ClientHandler.h"
#include "AMFWriter.h"
#include "AMFReader.h"
#include "Streams.h"

namespace Cumulus {

class ServerHandler
{
public:
	ServerHandler(Poco::UInt8 keepAliveServer,Poco::UInt8 keepAlivePeer,ClientHandler* pClientHandler);
	virtual ~ServerHandler();

	Group&	group(const std::vector<Poco::UInt8>& id);

	bool connection(Peer& peer);
	void failed(Peer& peer,const std::string& msg);
	void disconnection(Peer& peer);

	Streams				streams;

	const Poco::UInt8   id[32];
	const Poco::UInt32	keepAlivePeer;
	const Poco::UInt32	keepAliveServer;
private:
	ClientHandler*					_pClientHandler;
	std::list<Group*>				_groups;
};


} // namespace Cumulus
