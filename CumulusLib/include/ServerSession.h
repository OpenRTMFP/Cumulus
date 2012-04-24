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
#include "Session.h"
#include "FlowNull.h"
#include "Target.h"
#include "Invoker.h"
#include "Poco/Timestamp.h"

namespace Cumulus {

#define SESSION_BY_EDGE 1


class Attempt {
public:
	Attempt() : count(0) {
	}
	~Attempt() {
	}
	Poco::UInt32	count;

	bool obsolete() {
		return _time.isElapsed(120000000);
	}
private:
	Poco::Timestamp _time;
};

class ServerSession : public BandWriter,public Session {
public:

	ServerSession(ReceivingEngine& receivingEngine,
		     SendingEngine& sendingEngine,
			Poco::UInt32 id,
			Poco::UInt32 farId,
			const Peer& peer,
			const Poco::UInt8* decryptKey,
			const Poco::UInt8* encryptKey,
			Invoker& invoker);

	virtual ~ServerSession();

	PacketWriter&		writer();
	bool				failed() const;
	void				manage();
	void				kill();

	void				p2pHandshake(const Poco::Net::SocketAddress& address,const std::string& tag,Session* pSession);

	Poco::UInt32	helloAttempt(const std::string& tag);
	template<class AttemptType>
	AttemptType&	helloAttempt(const std::string& tag) {
		std::map<std::string,Attempt*>::iterator it = _helloAttempts.lower_bound(tag);
		if(it!=_helloAttempts.end() && it->first==tag) {
			++it->second->count;
			return (AttemptType&)*it->second;
		}
		if(it!=_helloAttempts.begin())
			--it;
		return (AttemptType&)*_helloAttempts.insert(it,std::pair<std::string,Attempt*>(tag,new AttemptType()))->second;
	}
	void			eraseHelloAttempt(const std::string& tag);
	
	// For middle peer/peer
	Target*	pTarget;
protected:
	

	virtual void	failSignal();
	void			fail(const std::string& fail);

	void			flush(bool echoTime=true);
	void			flush(bool echoTime,AESEngine::Type type);
	void			flush(Poco::UInt8 marker,bool echoTime);
	void			flush(Poco::UInt8 marker,bool echoTime,AESEngine::Type type);

	Invoker&					_invoker; // Protected for Middle session
	Poco::Timestamp				_recvTimestamp; // Protected for Middle session
	Poco::UInt16				_timeSent; // Protected for Middle session

private:
	void				packetHandler(PacketReader& packet);

	// Implementation of BandWriter
	void				initFlowWriter(FlowWriter& flowWriter);
	void				resetFlowWriter(FlowWriter& flowWriter);
	bool				canWriteFollowing(FlowWriter& flowWriter);

	PacketWriter&		writeMessage(Poco::UInt8 type,Poco::UInt16 length,FlowWriter* pFlowWriter=NULL);

	bool				keepAlive();

	FlowWriter*			flowWriter(Poco::UInt64 id);
	Flow&				flow(Poco::UInt64 id);
	Flow*				createFlow(Poco::UInt64 id,const std::string& signature);
	
	bool								_failed;
	Poco::UInt8							_timesFailed;
	Poco::UInt8							_timesKeepalive;

	std::map<Poco::UInt64,Flow*>		_flows;
	FlowNull*							_pFlowNull;
	std::map<Poco::UInt64,FlowWriter*>	_flowWriters;
	FlowWriter*							_pLastFlowWriter;
	Poco::UInt64						_nextFlowWriterId;

	std::map<std::string,Attempt*>		_helloAttempts;
};

inline Poco::UInt32	ServerSession::helloAttempt(const std::string& tag) {
	return (helloAttempt<Attempt>(tag)).count;
}

inline void ServerSession::flush(Poco::UInt8 marker,bool echoTime) {
	flush(marker,echoTime,prevAESType);
}

inline void ServerSession::flush(bool echoTime,AESEngine::Type type) {
	flush(0x4a,echoTime,type);
}

inline void ServerSession::flush(bool echoTime) {
	flush(0x4a,echoTime,prevAESType);
}

inline bool ServerSession::canWriteFollowing(FlowWriter& flowWriter) {
	return _pLastFlowWriter==&flowWriter;
}

inline bool ServerSession::failed() const {
	return _failed;
}


} // namespace Cumulus
