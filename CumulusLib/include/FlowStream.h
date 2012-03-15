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
	FlowStream(Poco::UInt64 id,const std::string& signature,Peer& peer,Invoker& invoker,BandWriter& band);
	virtual ~FlowStream();

	const std::string		name;

	static std::string	Signature;
private:
	enum StreamState {
		IDLE,
		PUBLISHING,
		PLAYING
	};

	static std::string	_Name;

	void rawHandler(Poco::UInt8 type,PacketReader& data);
	void audioHandler(PacketReader& packet);
	void videoHandler(PacketReader& packet);
	void messageHandler(const std::string& action,AMFReader& message);

	void commitHandler();
	void lostFragmentsHandler(Poco::UInt32 count);

	void disengage();

	Poco::UInt32	_index;

	Publication*	_pPublication;
	Listener*		_pListener;
	StreamState		_state;

	// Lost Fragments
	bool			_isVideo;
	Poco::UInt32	_numberLostFragments;

	BinaryWriter& write();
};

inline BinaryWriter& FlowStream::write() {
	return writer.writeRawMessage(true);
}

} // namespace Cumulus
