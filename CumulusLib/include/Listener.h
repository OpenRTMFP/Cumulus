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
#include "FlowWriter.h"
#include "QualityOfService.h"

namespace Cumulus {

class Publication;
class AudioWriter;
class VideoWriter;
class Listener {
public:
	Listener(Poco::UInt32 id,Publication& publication,FlowWriter& writer,bool unbuffered);
	virtual ~Listener();

	void startPublishing(const std::string& name); 
	void stopPublishing(const std::string& name); 

	void sampleAccess(bool audio,bool video) const;

	void pushAudioPacket(Poco::UInt32 time,PacketReader& packet); 
	void pushVideoPacket(Poco::UInt32 time,PacketReader& packet);
	void pushDataPacket(const std::string& name,PacketReader& packet);

	void flush();

	const Publication&	publication;
	const Poco::UInt32  id;

	const bool audioSampleAccess;
	const bool videoSampleAccess;

	const QualityOfService&	videoQOS() const;
	const QualityOfService&	audioQOS() const;

	void init();

private:
	Poco::UInt32 	computeTime(Poco::UInt32 time);

	void			writeBounds();
	void			writeBound(FlowWriter& writer);

	bool					_unbuffered;
	Poco::UInt32			_boundId;

	bool					_firstKeyFrame;

	Poco::UInt32 			_deltaTime;
	Poco::UInt32 			_addingTime;
	Poco::UInt32 			_time;

	FlowWriter&				_writer;
	AudioWriter*			_pAudioWriter;
	VideoWriter*			_pVideoWriter;
};


} // namespace Cumulus
