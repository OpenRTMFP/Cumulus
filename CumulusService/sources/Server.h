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

#include "RTMFPServer.h"
#include "Auth.h"
#include "StatusWriter.h"
#include "ApplicationKiller.h"

class Server : public Cumulus::RTMFPServer {
public:
	Server(Auth& auth,ApplicationKiller& applicationKiller);
	virtual ~Server();

private:
	void manage();

	//events
	void    onStart();
	void    onStop();

	bool	onConnection(Cumulus::Client& client,Cumulus::FlowWriterFactory& flowWriterFactory);
	void	onFailed(const Cumulus::Client& client,const std::string& error);
	void	onDisconnection(const Cumulus::Client& client);
	bool	onMessage(const Cumulus::Client& client,const std::string& name,Cumulus::AMFReader& reader,Cumulus::FlowWriter& writer);

	void	onPublish(const Cumulus::Client& client,const Cumulus::Publication& publication);
	void	onUnpublish(const Cumulus::Client& client,const Cumulus::Publication& publication);

	void	onAudioPacket(const Cumulus::Client& client,const Cumulus::Publication& publication,Poco::UInt32 time,Cumulus::PacketReader& packet);
	void	onVideoPacket(const Cumulus::Client& client,const Cumulus::Publication& publication,Poco::UInt32 time,Cumulus::PacketReader& packet);

	void	onSubscribe(const Cumulus::Client& client,const Cumulus::Listener& listener);
	void	onUnsubscribe(const Cumulus::Client& client,const Cumulus::Listener& listener);

	Auth&														_auth;
	std::map<const Cumulus::Client* const,StatusWriter*>		_status;

	ApplicationKiller&											_applicationKiller;
};
