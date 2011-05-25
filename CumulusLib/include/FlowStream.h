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
#include "Flow.h"
#include "Publication.h"

namespace Cumulus {

class FlowStream : public Flow {
public:
	FlowStream(Poco::UInt32 id,const std::string& signature,Peer& peer,ServerHandler& serverHandler,BandWriter& band);
	virtual ~FlowStream();

	const std::string		name;

	static std::string	s_signature;
private:
	enum StreamState {
		IDLE,
		PUBLISHING,
		PLAYING
	};

	static std::string	s_name;

	void rawHandler(Poco::UInt8 type,PacketReader& data);
	void audioHandler(PacketReader& packet);
	void videoHandler(PacketReader& packet);
	void messageHandler(const std::string& action,AMFReader& message);

	Poco::UInt32	_index;

	Publication*	_pPublication;
	Listener*		_pListener;
	StreamState		_state;

	BinaryWriter& write();
};

inline BinaryWriter& FlowStream::write() {
	return writer.writeRawMessage(true);
}


} // namespace Cumulus
