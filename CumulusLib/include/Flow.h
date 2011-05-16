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
#include "Peer.h"
#include "ServerHandler.h"
#include "FlowWriter.h"

namespace Cumulus {

class Flow {
public:
	Flow(Poco::UInt32 id,const std::string& signature,const std::string& name,Peer& peer,ServerHandler& serverHandler,BandWriter& band);
	virtual ~Flow();

	const Poco::UInt32		id;

	virtual void			messageHandler(Poco::UInt32 stage,PacketReader& message,Poco::UInt8 flags);

	void			fail(const std::string& error);
	void			flush();

	template <class FlowWriterType>
	FlowWriterType& newFlowWriter(const std::string& signature) {
		return *(new FlowWriterType(id,signature,_band));
	}

	bool			consumed();
	Poco::UInt32		stage();

protected:
	virtual void messageHandler(const std::string& name,AMFReader& message);
	virtual void rawHandler(Poco::UInt8 type,PacketReader& data);
	virtual void audioHandler(PacketReader& packet);
	virtual void videoHandler(PacketReader& packet);

	virtual void complete();
	
	Peer&					peer;
	FlowWriter&				writer;
	ServerHandler&			serverHandler;
	
private:
	
	Poco::UInt8			unpack(PacketReader& reader);

	bool				_completed;
	const std::string&	_name;
	const std::string&	_signature;
	BandWriter&			_band;

	// Receiving
	Poco::UInt32		_stage;
	Poco::UInt8*		_pBuffer;
	Poco::UInt32		_sizeBuffer;
};

inline Poco::UInt32 Flow::stage() {
	return _stage;
}

inline bool Flow::consumed() {
	return _completed;
}

} // namespace Cumulus
