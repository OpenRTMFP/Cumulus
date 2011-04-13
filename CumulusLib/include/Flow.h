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
#include "Message.h"
#include "AMFReader.h"
#include "AMFObjectWriter.h"
#include "Trigger.h"
#include "Peer.h"
#include "Group.h"
#include "ServerHandler.h"

#define MESSAGE_HEADER			0x80
#define MESSAGE_WITH_AFTERPART  0x10 
#define MESSAGE_WITH_BEFOREPART	0x20
#define MESSAGE_END				0x03

namespace Cumulus {

class Session;
class Flow {
public:
	Flow(Poco::UInt8 id,const std::string& signature,const std::string& name,Peer& peer,Session& session,ServerHandler& serverHandler);
	virtual ~Flow();

	void messageHandler(Poco::UInt32 stage,PacketReader& message,Poco::UInt8 flags);

	void flush();
	void flushMessages();

	void acknowledgment(Poco::UInt32 stage);
	bool consumed();
	void fail();
	void raise();
	virtual void complete();

	Poco::UInt32		stageRcv();
	Poco::UInt32		stageSnd();

	const Poco::UInt8		id;

	BinaryWriter&			writeRawMessage(bool withoutHeader=false);
	AMFWriter&				writeAMFMessage();

	AMFObjectWriter			writeSuccessResponse(const std::string& description,const std::string& name="Success");
	AMFObjectWriter			writeStatusResponse(const std::string& name,const std::string& description);
	AMFObjectWriter			writeErrorResponse(const std::string& description,const std::string& name="Failed");

protected:
	virtual void messageHandler(const std::string& name,AMFReader& message);
	virtual void rawHandler(Poco::UInt8 type,PacketReader& data);
	virtual void audioHandler(PacketReader& packet);
	virtual void videoHandler(PacketReader& packet);


	Peer&					peer;

	ServerHandler&			serverHandler;
	
private:
	void raiseMessage();

	void fillCode(const std::string& name,std::string& code);

	Message&	createMessage();
	Poco::UInt8 unpack(PacketReader& reader);

	bool				_completed;
	const std::string&	_name;
	const std::string&	_signature;
	Session&			_session;

	// Receiving
	Poco::UInt32		_stageRcv;
	Poco::UInt8*		_pBuffer;
	Poco::UInt32		_sizeBuffer;

	// Sending
	Poco::UInt32			_stageSnd;
	std::list<Message*>		_messages;
	double					_callbackHandle;
	std::string				_code;
	Trigger					_trigger;
	Message					_messageNull;
};

inline Poco::UInt32 Flow::stageRcv() {
	return _stageRcv;
}
inline Poco::UInt32 Flow::stageSnd() {
	return _stageSnd;
}

inline void Flow::complete() {
	_completed = true;
}

inline bool Flow::consumed() {
	return _completed && _messages.empty();
}

} // namespace Cumulus
