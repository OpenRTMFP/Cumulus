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
#include "BandWriter.h"

#define MESSAGE_HEADER			0x80
#define MESSAGE_WITH_AFTERPART  0x10 
#define MESSAGE_WITH_BEFOREPART	0x20
#define MESSAGE_ABANDONMENT		0x02
#define MESSAGE_END				0x01

namespace Cumulus {

class Invoker;
class FlowWriter {
	friend class Flow;
public:
	FlowWriter(const std::string& signature,BandWriter& band);
	virtual ~FlowWriter();

	const Poco::UInt64		id;
	const bool				critical;
	const Poco::UInt64		flowId;
	const std::string		signature;

	template<class FlowWriterType>
	FlowWriterType& newFlowWriter() {
		return *(new FlowWriterType(signature,_band));
	}

	void			flush(bool full=false);

	void			acknowledgment(PacketReader& reader);
	virtual void	manage(Invoker& invoker);

	bool			closed();
	void			fail(const std::string& error);
	void			clear();
	void			close();
	bool			consumed();

	void			cancel(Poco::UInt32 index);
	Poco::UInt32	queue();

	Poco::UInt64	stage();

	void			writeUnbufferedMessage(const Poco::UInt8* data,Poco::UInt32 size,const Poco::UInt8* memAckData=NULL,Poco::UInt32 memAckSize=0);

	BinaryWriter&	writeRawMessage(bool withoutHeader=false);

	AMFWriter&		writeAMFPacket(const std::string& name);
	AMFWriter&		writeAMFMessage(const std::string& name);

	AMFWriter&		writeAMFResult();
	AMFObjectWriter	writeSuccessResponse(const std::string& code,const std::string& description);
	AMFObjectWriter	writeStatusResponse(const std::string& code,const std::string& description);
	AMFObjectWriter	writeErrorResponse(const std::string& code,const std::string& description);

private:
	FlowWriter(FlowWriter& flowWriter);

	void					release();

	void					writeResponseHeader(BinaryWriter& writer,const std::string& name,double callbackHandle);
	AMFObjectWriter			writeAMFResponse(const std::string& name,const std::string& code,const  std::string& description);
	
	Poco::UInt32			headerSize(Poco::UInt64 stage);
	void					flush(PacketWriter& writer,Poco::UInt64 stage,Poco::UInt8 flags,bool header,BinaryReader& reader,Poco::UInt16 size);

	virtual void			ackMessageHandler(Poco::UInt32 ackCount,Poco::UInt32 lostCount,BinaryReader& content,Poco::UInt32 available,Poco::UInt32 size);
	virtual void			reset(Poco::UInt32 count){}
	void					raiseMessage();
	MessageBuffered&		createBufferedMessage();
	void					writeAbandonMessage();

	BandWriter&				_band;
	bool					_closed;
	Trigger					_trigger;

	
	std::list<Message*>		_messages;
	Poco::UInt64			_stage;
	std::list<Message*>		_messagesSent;
	Poco::UInt64			_stageAck;
	Poco::UInt32			_lostCount;
	Poco::UInt32			_ackCount;
	Poco::UInt32			_repeatable;

	// For single thread AMF response!
	double					_callbackHandle;
	std::string				_obj;

	Poco::UInt32			_resetCount;
	static MessageNull		_MessageNull;
};

inline void FlowWriter::writeAbandonMessage() {
	createBufferedMessage();
}

inline Poco::UInt32 FlowWriter::queue() {
	return _messages.size();
}

inline bool FlowWriter::closed() {
	return _closed;
}

inline void FlowWriter::ackMessageHandler(Poco::UInt32 ackCount,Poco::UInt32 lostCount,BinaryReader& content,Poco::UInt32 available,Poco::UInt32 size) {}

inline AMFObjectWriter FlowWriter::writeSuccessResponse(const std::string& code,const  std::string& description) {
	return writeAMFResponse("_result",code,description);
}
inline AMFObjectWriter FlowWriter::writeStatusResponse(const  std::string& code,const  std::string& description) {
	return writeAMFResponse("onStatus",code,description);
}
inline AMFObjectWriter FlowWriter::writeErrorResponse(const  std::string& code,const  std::string& description) {
	return writeAMFResponse("_error",code,description);
}

inline Poco::UInt64 FlowWriter::stage() {
	return _stage;
}
inline bool FlowWriter::consumed() {
	return _messages.empty() && _closed;
}

} // namespace Cumulus
