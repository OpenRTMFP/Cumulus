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

#include "Listener.h"
#include "Poco/StreamCopier.h"

using namespace Poco;
using namespace std;

namespace Cumulus {

class StreamWriter : public FlowWriter {
public:
	StreamWriter(UInt8 type,const string& signature,BandWriter& band) : FlowWriter(signature,band),_type(type),reseted(false) {	}
	~StreamWriter() {}

	void write(UInt32 time,PacketReader& data,bool unbuffered) {
	//	if(_type==0x08)
	//		time=0;
	/*	if(_type==0x09)
			TRACE("Video timestamp : %u",_time)
		else
			TRACE("Audio timestamp : %u",_time);*/
		if(unbuffered) {
			if(data.position()>=5) {
				data.reset(data.position()-5);
				PacketWriter writer(data.current(),5);
				writer.write8(_type);
				writer.write32(time);
				writeUnbufferedMessage(data.current(),data.available(),data.current(),5);
				return;
			}
			WARN("Written unbuffered impossible, it requires 5 head bytes available on PacketReader given");
		}
		BinaryWriter& out = writeRawMessage(true);
		out.write8(_type);
		out.write32(time);
		StreamCopier::copyStream(data.stream(),out.stream());

	}

	QualityOfService	qos;
	bool				reseted;

private:
	void ackMessageHandler(UInt32 ackCount,UInt32 lostCount,BinaryReader& content,UInt32 size) {
		if(content.read8()!=_type)
			return;
		qos.add(content.read32(),ackCount,lostCount);
	}

	// call on FlowWriter failed, we must rewritting bound infos
	void reset(UInt32 count) {
		reseted=true;
		qos.reset();
	}

	UInt8 _type;
};

class AudioWriter : public StreamWriter {
public:
	AudioWriter(const string& signature,BandWriter& band) : StreamWriter(0x08,signature,band){}
};

class VideoWriter : public StreamWriter {
public:
	VideoWriter(const string& signature,BandWriter& band) : StreamWriter(0x09,signature,band){}
};

Listener::Listener(UInt32 id,Publication& publication,FlowWriter& writer,bool unbuffered) :
	_unbuffered(unbuffered),_writer(writer),_boundId(0),
	id(id),publication(publication),_firstKeyFrame(false),
	_audioWriter(writer.newFlowWriter<AudioWriter>()),
	_videoWriter(writer.newFlowWriter<VideoWriter>()),
	_time(0),_deltaTime(0),_addingTime(0) {

	writeBounds();
}

Listener::~Listener() {
	_audioWriter.close();
	_videoWriter.close();
}

const QualityOfService& Listener::audioQOS() const {
	return _audioWriter.qos;
}

const QualityOfService& Listener::videoQOS() const {
	return _videoWriter.qos;
}

UInt32 Listener::computeTime(UInt32 time) {
	if(time==0)
		time=1;
	if(_deltaTime==0 && _addingTime==0) {
		_deltaTime = time;
		DEBUG("Deltatime assignment : %u",_deltaTime);
	}
	if(_deltaTime>time) {
		WARN("Time infererior to deltaTime on listener %u, certainly a non increasing time",id);
		_deltaTime=time;
	}
	_time = time-_deltaTime+_addingTime;
	return _time;
}

void Listener::writeBound(FlowWriter& writer) {
	DEBUG("Writing bound %u on flow writer %u",_boundId,writer.id);
	BinaryWriter& data = writer.writeRawMessage();
	data.write16(0x22);
	data.write32(_boundId);
	data.write32(3); // 3 tracks!
}

void Listener::writeBounds() {
	writeBound(_videoWriter);
	writeBound(_audioWriter);
	writeBound(_writer);
	++_boundId;
}

void Listener::startPublishing(const string& name) {
	_writer.writeStatusResponse("Play.PublishNotify",name +" is now published");
	_firstKeyFrame=false;
}

void Listener::stopPublishing(const string& name) {
	_writer.writeStatusResponse("Play.UnpublishNotify",name +" is now unpublished");
	_deltaTime=0;
	_addingTime = _time;
	_audioWriter.qos.reset();
	_videoWriter.qos.reset();
}


void Listener::pushDataPacket(const string& name,PacketReader& packet) {
	// TODO create _dataWriter ??
	if(_unbuffered) {
		UInt16 offset = name.size()+9;
		if(packet.position()>=offset) {
			packet.reset(packet.position()-offset);
			_writer.writeUnbufferedMessage(packet.current(),packet.available());
			return;
		}
		WARN("Written unbuffered impossible, it requires %u head bytes available on PacketReader given",offset);
	}
	StreamCopier::copyStream(packet.stream(),_writer.writeStreamData(name).writer.stream());
}

void Listener::pushVideoPacket(UInt32 time,PacketReader& packet) {
	// key frame ?
	if(((*packet.current())&0xF0) == 0x10)
		_firstKeyFrame=true;

	if(!_firstKeyFrame) {
		DEBUG("Video frame dropped for listener %u to wait first key frame",id);
		++(UInt32&)_videoWriter.qos.droppedFrames;
		return;
	}

	if(_videoWriter.reseted) {
		_videoWriter.reseted=false;
		writeBounds();
	}

	_videoWriter.write(computeTime(time),packet,_unbuffered);
}


void Listener::pushAudioPacket(UInt32 time,PacketReader& packet) {
	if(_audioWriter.reseted) {
		_audioWriter.reseted=false;
		writeBounds();
	}
	_audioWriter.write(computeTime(time),packet,_unbuffered);
}

void Listener::flush() {
	_audioWriter.flush();
	_videoWriter.flush();
	_writer.flush(true);
}


} // namespace Cumulus
