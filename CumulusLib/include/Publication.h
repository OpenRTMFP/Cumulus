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
#include "Clients.h"

namespace Cumulus {

class Handler;
class CUMULUS_API Publication {
	friend class Publications;
public:
	Publication(const std::string& name,Handler& handler);
	virtual ~Publication();

	Poco::UInt32			publisherId() const;
	const std::string&		name() const;

	const Listeners			listeners;

	const QualityOfService&	videoQOS() const;
	const QualityOfService&	audioQOS() const;

	bool					start(Client& client,Poco::UInt32	publisherId);
	void					stop(Client& client,Poco::UInt32	publisherId);

	void					pushAudioPacket(const Client& client,Poco::UInt32 time,PacketReader& packet,Poco::UInt32 numberLostFragments=0);
	void					pushVideoPacket(const Client& client,Poco::UInt32 time,PacketReader& packet,Poco::UInt32 numberLostFragments=0);
	void					pushDataPacket(const Client& client,const std::string& name,PacketReader& packet);

	void					addListener(Client& client,Poco::UInt32 id,FlowWriter& writer,bool unbuffered);
	void					removeListener(Client& client,Poco::UInt32 id);

	void					flush();
private:
	bool								_firstKeyFrame;
	Poco::UInt32						_time;
	std::string							_name;
	Poco::UInt32						_publisherId;
	std::map<Poco::UInt32,Listener*>	_listeners;
	Handler&							_handler;

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
