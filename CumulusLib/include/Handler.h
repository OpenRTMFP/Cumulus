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
#include "Invoker.h"

namespace Cumulus {

class Handler : public Invoker {
public:
	//events
	virtual	void			onRendezVousUnknown(const Poco::UInt8* id,std::set<std::string>& addresses){}
	virtual void			onHandshake(Poco::UInt32 attempts,const Poco::Net::SocketAddress& address,const std::string& path,const std::map<std::string,std::string>& properties,std::set<std::string>& addresses){}
	virtual bool			onConnection(Client& client,AMFReader& parameters,AMFObjectWriter& response){return true;}
	virtual void			onFailed(const Client& client,const std::string& error){}
	virtual void			onDisconnection(const Client& client){}
	virtual bool			onMessage(Client& client,const std::string& name,AMFReader& reader){return false;}

	virtual void			onJoinGroup(Client& client,Group& group){}
	virtual void			onUnjoinGroup(Client& client,Group& group){}

	virtual bool			onPublish(Client& client,const Publication& publication,std::string& error){return true;}
	virtual void			onUnpublish(Client& client,const Publication& publication){}

	virtual void			onDataPacket(Client& client,const Publication& publication,const std::string& name,PacketReader& packet){}
	virtual void			onAudioPacket(Client& client,const Publication& publication,Poco::UInt32 time,PacketReader& packet){}
	virtual void			onVideoPacket(Client& client,const Publication& publication,Poco::UInt32 time,PacketReader& packet){}

	virtual bool			onSubscribe(Client& client,const Listener& listener,std::string& error){return true;}
	virtual void			onUnsubscribe(Client& client,const Listener& listener){}

	virtual void			onManage(Client& client){}
protected:
	Handler(Poco::UInt32 threads):_myself(*this),Invoker(threads) {(bool&)_myself.connected=true;}
	virtual ~Handler(){}
private:
	Peer&					myself();
	Peer					_myself;
};

inline Peer& Handler::myself() {
	return _myself;
}



} // namespace Cumulus
