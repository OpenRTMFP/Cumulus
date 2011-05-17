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
#define MESSAGE_END				0x03

namespace Cumulus {

class FlowWriter {
	friend class Flow;
public:
	FlowWriter(Poco::UInt32 flowId,const std::string& signature,BandWriter& band);
	virtual ~FlowWriter();

	const Poco::UInt32		id;
	const Poco::UInt32		flowId;
	const std::string&		signature;

	void flush();

	void acknowledgment(Poco::UInt32 stage);
	void raise();

	void close();
	bool consumed();

	Poco::UInt32		stage();

	BinaryWriter&			writeRawMessage(bool withoutHeader=false);
	AMFWriter&				writeAMFMessage(const std::string& name);
	AMFWriter&				writeAMFResult();

	AMFObjectWriter			writeSuccessResponse(const std::string& description,const std::string& name="Success");
	AMFObjectWriter			writeStatusResponse(const std::string& name,const std::string& description);
	AMFObjectWriter			writeErrorResponse(const std::string& description,const std::string& name="Failed");

	
private:
	void					raiseMessage();

	virtual Message&		createMessage();

	
	bool					_closed;
	Poco::UInt32			_stage;
	std::list<Message*>		_messages;
	Trigger					_trigger;
	BandWriter&				_band;
	Message					_messageNull;

	// For single thread AMF response!
	double					_callbackHandle;
	std::string				_code;
};

inline AMFWriter& FlowWriter::writeAMFResult() {
	return writeAMFMessage("_result");
}
inline Poco::UInt32 FlowWriter::stage() {
	return _stage;
}
inline bool FlowWriter::consumed() {
	return _closed && _messages.empty();
}

} // namespace Cumulus
