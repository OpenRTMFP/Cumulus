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
#include "ApplicationKiller.h"
#include "Service.h"
#include "Blacklist.h"
#include "SMTPSession.h"
#include "TCPServer.h"

class Server : public Cumulus::RTMFPServer {
public:
	Server(ApplicationKiller& applicationKiller,const Poco::Util::AbstractConfiguration& configurations);
	virtual ~Server();

	static const std::string				WWWPath;
	SMTPSession								mails;

private:
	void					handle(bool& terminate);
	void					manage();
	bool					readNextConfig(lua_State* pState,const Poco::Util::AbstractConfiguration& configurations,const std::string& root);

	//events
	void					onStart();
	void					onStop();

	bool					onConnection(Cumulus::Client& client,Cumulus::AMFReader& parameters,Cumulus::AMFObjectWriter& response);
	void					onFailed(const Cumulus::Client& client,const std::string& error);
	void					onDisconnection(const Cumulus::Client& client);
	bool					onMessage(Cumulus::Client& client,const std::string& name,Cumulus::AMFReader& reader);

	void					onJoinGroup(Cumulus::Client& client,Cumulus::Group& group);
	void					onUnjoinGroup(Cumulus::Client& client,Cumulus::Group& group);

	bool					onPublish(Cumulus::Client& client,const Cumulus::Publication& publication,std::string& error);
	void					onUnpublish(Cumulus::Client& client,const Cumulus::Publication& publication);

	void					onAudioPacket(Cumulus::Client& client,const Cumulus::Publication& publication,Poco::UInt32 time,Cumulus::PacketReader& packet);
	void					onVideoPacket(Cumulus::Client& client,const Cumulus::Publication& publication,Poco::UInt32 time,Cumulus::PacketReader& packet);
	void					onDataPacket(Cumulus::Client& client,const Cumulus::Publication& publication,const std::string& name,Cumulus::PacketReader& packet);

	bool					onSubscribe(Cumulus::Client& client,const Cumulus::Listener& listener,std::string& error);
	void					onUnsubscribe(Cumulus::Client& client,const Cumulus::Listener& listener);

	Blacklist				_blacklist;
	lua_State*				_pState;
	ApplicationKiller&		_applicationKiller;
	Service*				_pService;
};
