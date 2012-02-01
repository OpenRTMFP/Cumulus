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
#include "Listeners.h"
#include "Peer.h"

namespace Cumulus {

class Publication {
public:
	Publication(const std::string& name);
	virtual ~Publication();

	Poco::UInt32			publisherId() const;
	const std::string&		name() const;

	const Listeners			listeners;

	const QualityOfService&	videoQOS() const;
	const QualityOfService&	audioQOS() const;

	void					closePublisher(const std::string& code="",const std::string& description="");

	void					start(Peer& peer,Poco::UInt32	publisherId,FlowWriter* pWriter);
	void					stop(Peer& peer,Poco::UInt32	publisherId);

	void					pushAudioPacket(Poco::UInt32 time,PacketReader& packet,Poco::UInt32 numberLostFragments=0);
	void					pushVideoPacket(Poco::UInt32 time,PacketReader& packet,Poco::UInt32 numberLostFragments=0);
	void					pushDataPacket(const std::string& name,PacketReader& packet);

	bool					addListener(Peer& peer,Poco::UInt32 id,FlowWriter& writer,bool unbuffered);
	void					removeListener(Peer& peer,Poco::UInt32 id);

	void					flush();
private:
	Peer*								_pPublisher;
	FlowWriter*							_pController;
	bool								_firstKeyFrame;
	std::string							_name;
	Poco::UInt32						_publisherId;
	std::map<Poco::UInt32,Listener*>	_listeners;

	QualityOfService					_videoQOS;
	QualityOfService					_audioQOS;
};

inline const QualityOfService& Publication::audioQOS() const {
	return _audioQOS;
}

inline const QualityOfService& Publication::videoQOS() const {
	return _videoQOS;
}

inline Poco::UInt32 Publication::publisherId() const {
	return _publisherId;
}

inline const std::string& Publication::name() const {
	return _name;
}

} // namespace Cumulus
