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
#include "PacketReader.h"
#include "PacketWriter.h"
#include "AESEngine.h"
#include "RTMFP.h"
#include "Flow.h"
#include "FlowNull.h"
#include "Target.h"
#include "BandWriter.h"
#include "FlowWriter.h"
#include "Poco/Timestamp.h"
#include "Poco/Net/DatagramSocket.h"

namespace Cumulus {

class Session : public BandWriter {
public:

	Session(Poco::UInt32 id,
			Poco::UInt32 farId,
			const Peer& peer,
			const Poco::UInt8* decryptKey,
			const Poco::UInt8* encryptKey,
			Poco::Net::DatagramSocket& socket,
			Handler& handler);

	virtual ~Session();

	virtual void	packetHandler(PacketReader& packet);

	Poco::UInt32		id() const;
	Poco::UInt32		farId() const;
	const Peer& 		peer() const;
	bool				died() const;
	bool				failed() const;
	virtual void		manage();

	//Flow&	newFlow(const std::string& signature);

	PacketWriter&		writer();
	void				flush(Poco::UInt8 flags=0);

	void	p2pHandshake(const Poco::Net::SocketAddress& address,const std::string& tag,Session* pSession);
	bool	decode(PacketReader& packet,const Poco::Net::SocketAddress& sender);
	
	void	fail(const std::string& fail);

	const bool checked;

	// For middle peer/peer
	Target*	pTarget;
protected:
	
	virtual void	failSignal();
	void			kill();

	Handler&			_handler;

	Poco::UInt32			_farId; // Protected for Middle session
	Poco::Timestamp			_recvTimestamp; // Protected for Middle session
	Poco::UInt16			_timeSent; // Protected for Middle session

private:
	// Implementation of BandWriter
	void				initFlowWriter(FlowWriter& flowWriter);
	void				resetFlowWriter(FlowWriter& flowWriter);
	bool				canWriteFollowing(FlowWriter& flowWriter);

	PacketWriter&		writeMessage(Poco::UInt8 type,Poco::UInt16 length,FlowWriter* pFlowWriter=NULL);

	bool				keepAlive();

	FlowWriter*			flowWriter(Poco::UInt32 id);
	Flow&				flow(Poco::UInt32 id);
	Flow*				createFlow(Poco::UInt32 id,const std::string& signature);
	
	bool						_failed;
	Poco::UInt8					_timesFailed;
	Poco::UInt8					_timesKeepalive;

	std::map<Poco::UInt32,Flow*>		_flows;
	FlowNull*							_pFlowNull;
	std::map<Poco::UInt32,FlowWriter*>	_flowWriters;
	FlowWriter*							_pLastFlowWriter;
	Poco::UInt32						_nextFlowWriterId;

	Poco::UInt32				_id;

	Poco::Net::DatagramSocket&	_socket;
	AESEngine					_aesDecrypt;
	AESEngine					_aesEncrypt;

	Poco::UInt8					_buffer[PACKETSEND_SIZE];
	PacketWriter				_writer;

	bool						_died;
	Peer						_peer;

	std::map<std::string,Poco::UInt8>		_p2pHandshakeAttemps;
};

inline bool Session::canWriteFollowing(FlowWriter& flowWriter) {
	return _pLastFlowWriter==&flowWriter;
}


inline bool Session::failed() const {
	return _failed;
}

inline const Peer& Session::peer() const {
	return _peer;
}

inline Poco::UInt32 Session::id() const {
	return _id;
}

inline Poco::UInt32 Session::farId() const {
	return _farId;
}

inline bool Session::died() const {
	return _died;
}

} // namespace Cumulus
