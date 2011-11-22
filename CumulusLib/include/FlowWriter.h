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

class Handler;
class CUMULUS_API FlowWriter {
	friend class Flow;
public:
	FlowWriter(const std::string& signature,BandWriter& band);
	virtual ~FlowWriter();

	const Poco::UInt32		id;
	const bool				critical;
	const Poco::UInt32		flowId;
	const std::string		signature;

	template<class FlowWriterType>
	FlowWriterType& newFlowWriter() {
		return *(new FlowWriterType(signature,_band));
	}
	template<class FlowWriterType,class FlowWriterFactoryType>
	FlowWriterType& newFlowWriter(FlowWriterFactoryType& flowWriterFactory) {
		return flowWriterFactory.newFlowWriter(signature,_band);
	}

	void			flush(bool full=false);

	void			acknowledgment(PacketReader& reader);
	virtual void	manage(Handler& handler);

	bool			closed();
	void			fail(const std::string& error);
	void			clear();
	void			close();
	bool			consumed();

	Poco::UInt32	stage();

	void			repeat(Poco::UInt32 stage,Poco::UInt32 count);

	void			writeUnbufferedMessage(const Poco::UInt8* data,Poco::UInt32 size,const Poco::UInt8* memAckData=NULL,Poco::UInt32 memAckSize=0);

	BinaryWriter&	writeRawMessage(bool withoutHeader=false);
	AMFWriter&		writeStreamData(const std::string& name);

	AMFWriter&		writeAMFMessage(const std::string& name);
	AMFWriter&		writeAMFResult();

	AMFObjectWriter	writeSuccessResponse(const std::string& code,const std::string& description);
	AMFObjectWriter	writeStatusResponse(const std::string& code,const std::string& description);
	AMFObjectWriter	writeErrorResponse(const std::string& code,const std::string& description);

private:
	FlowWriter(FlowWriter& flowWriter);

	AMFObjectWriter			writeAMFResponse(const std::string& name,const std::string& code,const  std::string& description);
	
	Poco::UInt32			headerSize(Poco::UInt32 stage);
	void					flush(PacketWriter& writer,Poco::UInt32 stage,Poco::UInt8 flags,bool header,BinaryReader& reader,Poco::UInt16 size);

	virtual void			ackMessageHandler(Poco::UInt32 ackCount,Poco::UInt32 lostCount,BinaryReader& content,Poco::UInt32 size);
	virtual void			reset(Poco::UInt32 count){}
	void					raiseMessage();
	MessageBuffered&		createBufferedMessage();

	BandWriter&				_band;
	bool					_closed;
	bool					_abandoned;
	Trigger					_trigger;

	
	std::list<Message*>		_messages;
	Poco::UInt32			_stage;
	std::list<Message*>		_messagesSent;
	Poco::UInt32			_stageAck;
	Poco::UInt32			_lostCount;
	Poco::UInt32			_ackCount;
	Poco::UInt32			_repeatable;

	// For single thread AMF response!
	double					_callbackHandle;
	std::string				_obj;

	Poco::UInt32			_resetCount;
	static MessageNull		_MessageNull;
};

inline bool FlowWriter::closed() {
	return _closed;
}

inline void FlowWriter::ackMessageHandler(Poco::UInt32 ackCount,Poco::UInt32 lostCount,BinaryReader& content,Poco::UInt32 size) {}

inline AMFWriter& FlowWriter::writeAMFResult() {
	return writeAMFMessage("_result");
}
inline AMFObjectWriter FlowWriter::writeSuccessResponse(const std::string& code,const  std::string& description) {
	return writeAMFResponse("_result",code,description);
}
inline AMFObjectWriter FlowWriter::writeStatusResponse(const  std::string& code,const  std::string& description) {
	return writeAMFResponse("onStatus",code,description);
}
inline AMFObjectWriter FlowWriter::writeErrorResponse(const  std::string& code,const  std::string& description) {
	return writeAMFResponse("_error",code,description);
}

inline Poco::UInt32 FlowWriter::stage() {
	return _stage;
}
inline bool FlowWriter::consumed() {
	return _messages.empty() && _closed;
}

} // namespace Cumulus
