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
#include "Publication.h"
#include "Poco/StreamCopier.h"
#include "Poco/NumberFormatter.h"

using namespace Poco;
using namespace std;

namespace Cumulus {

class StreamWriter : public FlowWriter {
public:
	StreamWriter(UInt8 type,const string& signature,BandWriter& band) : FlowWriter(signature,band),_type(type),reseted(false),pClient(NULL) {	}
	~StreamWriter() {}

	void write(UInt32 time,PacketReader& data,bool unbuffered) {
	//	if(_type==0x08)
	//		time=0;
		if(_type==0x09)
			TRACE("Video timestamp : %u",time)
		else
			TRACE("Audio timestamp : %u",time);
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
	const Client*		pClient;

private:
	void ackMessageHandler(UInt32 ackCount,UInt32 lostCount,BinaryReader& content,UInt32 available,UInt32 size) {
		if(available==0 || content.read8()!=_type)
			return;
		qos.add(content.read32(),ackCount,lostCount,size,pClient ? pClient->ping : 0);
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
	_unbuffered(unbuffered),_writer(writer),_boundId(0),audioSampleAccess(false),videoSampleAccess(false),
	id(id),publication(publication),_firstKeyFrame(false),receiveAudio(true),receiveVideo(true),
	_pAudioWriter(NULL),_pVideoWriter(NULL),
	_time(0),_deltaTime(-1),_addingTime(0),_firstAudio(true),_firstVideo(true) {
}

Listener::~Listener() {
	if(_pAudioWriter)
		_pAudioWriter->close();
	if(_pVideoWriter)
		_pVideoWriter->close();
}

void Listener::init(const Client& client) {
	if(!_pAudioWriter) {
		_pAudioWriter = &_writer.newFlowWriter<AudioWriter>();
		_pAudioWriter->pClient = &client;
	} else
		WARN("Listener %u audio track has already been initialized",id);
	if(!_pVideoWriter) {
		_pVideoWriter = &_writer.newFlowWriter<VideoWriter>();
		_pVideoWriter->pClient = &client;
	} else
		WARN("Listener %u video track has already been initialized",id);
	writeBounds();
}

void Listener::sampleAccess(bool audio,bool video) const {
	(bool&)videoSampleAccess = video;
	(bool&)audioSampleAccess = audio;
	AMFWriter& amf = _writer.writeAMFPacket("|RtmpSampleAccess");
	amf.writeBoolean(audioSampleAccess);
	amf.writeBoolean(videoSampleAccess);
}

const QualityOfService& Listener::audioQOS() const {
	if(!_pAudioWriter)
		return QualityOfService::QualityOfServiceNull;
	return _pAudioWriter->qos;
}

const QualityOfService& Listener::videoQOS() const {
	if(!_pVideoWriter)
		return QualityOfService::QualityOfServiceNull;
	return _pVideoWriter->qos;
}

UInt32 Listener::computeTime(UInt32 time) {
	if(_deltaTime<0) {
		_deltaTime = time;
		DEBUG("Deltatime assignment : %u",_deltaTime);
	}
	if(_deltaTime>time) {
		WARN("Time infererior to deltaTime on listener %u, certainly a non increasing time",id);
		_deltaTime=time;
	}
	_time = time-(UInt32)_deltaTime+_addingTime;
	return _time;
}

void Listener::writeBound(FlowWriter& writer) {
	DEBUG("Writing bound %u on flow writer %s",_boundId,NumberFormatter::format(writer.id).c_str());
	BinaryWriter& data = writer.writeRawMessage();
	data.write16(0x22);
	data.write32(_boundId);
	data.write32(3); // 3 tracks!
}

void Listener::writeBounds() {
	if(_pVideoWriter)
		writeBound(*_pVideoWriter);
	if(_pAudioWriter)
		writeBound(*_pAudioWriter);
	writeBound(_writer);
	++_boundId;
}

void Listener::startPublishing(const string& name) {
	_writer.writeStatusResponse("Play.PublishNotify",name +" is now published");
	_firstKeyFrame=false;
}

void Listener::stopPublishing(const string& name) {
	_writer.writeStatusResponse("Play.UnpublishNotify",name +" is now unpublished");
	_deltaTime=-1;
	_addingTime = _time;
	_pAudioWriter->qos.reset();
	_pVideoWriter->qos.reset();
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
	StreamCopier::copyStream(packet.stream(),_writer.writeAMFPacket(name).writer.stream());
}

void Listener::pushVideoPacket(UInt32 time,PacketReader& packet) {
	if(!receiveVideo) {
		_firstKeyFrame=false;
		_firstVideo=true;
		return;
	}
	if(!_pVideoWriter) {
		ERROR("Listener %u must be initialized before to be used",id);
		return;
	}
	// key frame ?
	if(((*packet.current())&0xF0) == 0x10)
		_firstKeyFrame=true;

	if(!_firstKeyFrame) {
		DEBUG("Video frame dropped for listener %u to wait first key frame",id);
		++(UInt32&)_pVideoWriter->qos.droppedFrames;
		return;
	}

	if(_pVideoWriter->reseted) {
		_pVideoWriter->reseted=false;
		writeBounds();
	}

	time = computeTime(time);

	if(_firstVideo) {
		_firstVideo=false;
		UInt32 size = publication.videoCodecBuffer().size();
		if(size>0) {
			PacketReader videoCodecPacket(&publication.videoCodecBuffer()[0],size);
			_pVideoWriter->write(time,videoCodecPacket,false);
		}
	}

	_pVideoWriter->write(time,packet,_unbuffered);
}


void Listener::pushAudioPacket(UInt32 time,PacketReader& packet) {
	if(!receiveAudio) {
		_firstAudio=true;
		return;
	}
	if(!_pAudioWriter) {
		ERROR("Listener %u must be initialized before to be used",id);
		return;
	}
	if(_pAudioWriter->reseted) {
		_pAudioWriter->reseted=false;
		writeBounds();
	}

	time = computeTime(time);

	if(_firstAudio) {
		_firstAudio=false;
		UInt32 size = publication.audioCodecBuffer().size();
		if(size>0) {
			PacketReader audioCodecPacket(&publication.audioCodecBuffer()[0],size);
			_pAudioWriter->write(time,audioCodecPacket,false);
		}
	}

	_pAudioWriter->write(time,packet,_unbuffered);
}

void Listener::flush() {
	if(_pAudioWriter)
		_pAudioWriter->flush();
	if(_pVideoWriter)
		_pVideoWriter->flush();
	_writer.flush(true);
}


} // namespace Cumulus
