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

class FlowWriterFactory {
	friend class FlowWriter;
public:
	template<class FlowWriterType>
	FlowWriterType& newFlowWriter() {
		return *(new FlowWriterType(signature,_band));
	}

	template<class FlowWriterType,class FlowWriterFactoryType>
	FlowWriterType& newFlowWriter(FlowWriterFactoryType& flowWriterFactory) {
		return flowWriterFactory.newFlowWriter(signature,_band);
	}

	const Poco::UInt32		flowId;
	const std::string		signature;
private:
	BandWriter&				_band;
	FlowWriterFactory(const std::string& signature,BandWriter& band) : flowId(0),_band(band),signature(signature){}
};

class Handler;
class CUMULUS_API FlowWriter : public FlowWriterFactory {
	friend class Flow;
public:
	FlowWriter(const std::string& signature,BandWriter& band);
	virtual ~FlowWriter();

	const Poco::UInt32		id;
	const bool				critical;

	void flush(bool full=false);

	void acknowledgment(Poco::UInt32 stage);
	virtual void manage(Handler& handler);

	bool		 closed();
	Poco::UInt32 count();
	void fail(const std::string& error);
	void close();
	bool consumed();

	Poco::UInt32		stage();

	void					writeUnbufferedMessage(const Poco::UInt8* data,Poco::UInt32 size,const Poco::UInt8* memAckData=NULL,Poco::UInt32 memAckSize=0);

	BinaryWriter&			writeRawMessage(bool withoutHeader=false);
	AMFWriter&				writeAMFMessage(const std::string& name);
	AMFWriter&				writeAMFResult();

	AMFObjectWriter			writeAMFResponse(const std::string& name,const std::string& code,const  std::string& description);
	AMFObjectWriter			writeSuccessResponse(const std::string& code,const std::string& description);
	AMFObjectWriter			writeStatusResponse(const std::string& code,const std::string& description);
	AMFObjectWriter			writeErrorResponse(const std::string& code,const std::string& description);

private:
	FlowWriter(FlowWriter& flowWriter);
	
	virtual void			ackMessageHandler(BinaryReader& content,Poco::UInt32 size,Poco::UInt32 lostMessages);
	virtual void			reset(Poco::UInt32 count){}
	void					raiseMessage();
	MessageBuffered&		createBufferedMessage();

	void					clearMessages(bool exceptLast=false);


	bool					_closed;
	Poco::UInt32			_stage;
	std::list<Message*>		_messages;
	Trigger					_trigger;
	MessageBuffered			_messageNull;
	Poco::UInt32			_lostMessages;

	// For single thread AMF response!
	double					_callbackHandle;
	std::string				_obj;
	Poco::UInt32			_resetCount;
};

inline bool FlowWriter::closed() {
	return _closed;
}

inline void FlowWriter::ackMessageHandler(BinaryReader& content,Poco::UInt32 size,Poco::UInt32 lostMessages) {}

inline Poco::UInt32 FlowWriter::count() {
	return _messages.size();
}
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
	return _closed && _messages.empty();
}

} // namespace Cumulus
