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
#include "Subscription.h"

namespace Cumulus {

class FlowStream : public Flow,private Listener {
public:
	FlowStream(Poco::UInt8 id,const std::string& signature,Peer& peer,Session& session,ServerHandler& serverHandler);
	virtual ~FlowStream();

	static std::string	s_signature;
private:
	enum StreamState {
		IDLE,
		PUBLISHING,
		PLAYING
	};

	void  complete();
	static std::string	s_name;

	void rawHandler(Poco::UInt8 type,PacketReader& data);
	void audioHandler(PacketReader& packet);
	void videoHandler(PacketReader& packet);
	void messageHandler(const std::string& name,AMFReader& message);

	std::string		_signature;
	Poco::UInt32	_index;
	Subscription*	_pSubscription;
	StreamState		_state;
	std::string		_name;

	BinaryWriter& writer();
	void flush();
};

inline BinaryWriter& FlowStream::writer() {
	return writeRawMessage(true);
}
inline void FlowStream::flush() {
	return Flow::flush();
}

} // namespace Cumulus
