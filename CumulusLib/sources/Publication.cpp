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

#include "Publication.h"
#include "Logs.h"
#include "Handler.h"
#include "Poco/StreamCopier.h"
#include "string.h"

using namespace std;
using namespace Poco;


namespace Cumulus {

Publication::Publication(const string& name,Handler& handler):_handler(handler),_publisherId(0),_name(name),_time(0),_firstKeyFrame(false),listeners(_listeners) {
	DEBUG("New publication %s",_name.c_str());
}

Publication::~Publication() {
	// delete _listeners!
	map<UInt32,Listener*>::iterator it;
	for(it=_listeners.begin();it!=_listeners.end();++it)
		delete it->second;

	DEBUG("Publication %s deleted",_name.c_str());
}


void Publication::addListener(Client& client,UInt32 id,FlowWriter& writer,bool unbuffered) {
	map<UInt32,Listener*>::iterator it = _listeners.lower_bound(id);
	if(it!=_listeners.end() && it->first==id) {
		WARN("Listener %u is already subscribed for publication %u",id,_publisherId);
		return;
	}
	if(it!=_listeners.begin())
		--it;
	Listener* pListener = new Listener(id,*this,writer,unbuffered);
	_listeners.insert(it,pair<UInt32,Listener*>(id,pListener));
	_handler.onSubscribe(client,*pListener);
}

void Publication::removeListener(Client& client,UInt32 id) {
	map<UInt32,Listener*>::iterator it = _listeners.find(id);
	if(it==_listeners.end()) {
		WARN("Listener %u is already unsubscribed of publication %u",id,_publisherId);
		return;
	}
	_handler.onUnsubscribe(client,*it->second);
	delete it->second;
	_listeners.erase(it);	
}

bool Publication::start(Client& client,UInt32 publisherId) {
	if(_publisherId!=0)
		return false; // has already a publisher
	_publisherId = publisherId;
	_firstKeyFrame=false;
	map<UInt32,Listener*>::const_iterator it;
	for(it=_listeners.begin();it!=_listeners.end();++it)
		it->second->startPublishing(_name);
	flush();
	_handler.onPublish(client,*this);
	return true;
}

void Publication::stop(Client& client,UInt32 publisherId) {
	if(_publisherId==0)
		return; // already done
	if(_publisherId!=publisherId) {
		WARN("Unpublish '%s' operation with a %u id different than its publisher %u id",_name.c_str(),publisherId,_publisherId);
		return;
	}
	map<UInt32,Listener*>::const_iterator it;
	for(it=_listeners.begin();it!=_listeners.end();++it)
		it->second->stopPublishing(_name);
	flush();
	_handler.onUnpublish(client,*this);
	_videoQOS.reset();
	_audioQOS.reset();
	_time=0;
	_publisherId = 0;
	return;
}

void Publication::flush() {
	map<UInt32,Listener*>::const_iterator it;
	for(it=_listeners.begin();it!=_listeners.end();++it)
		it->second->flush();
}

void Publication::pushDataPacket(const Client& client,const string& name,PacketReader& packet) {
	if(_publisherId==0) {
		ERROR("Data packet pushed on a publication %u who is idle",_publisherId);
		return;
	}
	int pos = packet.position();
	map<UInt32,Listener*>::const_iterator it;
	for(it=_listeners.begin();it!=_listeners.end();++it) {
		it->second->pushDataPacket(name,packet);
		packet.reset(pos);
	}
	_handler.onDataPacket(client,*this,name,packet);
}

void Publication::pushAudioPacket(const Client& client,UInt32 time,PacketReader& packet,UInt32 numberLostFragments) {
	if(_publisherId==0) {
		ERROR("Audio packet pushed on a publication %u who is idle",_publisherId);
		return;
	}

	if(time&0xFF000000) // TODO solve it!
		time = _time;
	_time = time;

	int pos = packet.position();
	if(numberLostFragments>0)
		INFO("%u audio fragments lost on publication %u",numberLostFragments,_publisherId);
	_audioQOS.add(_time,packet.fragments,numberLostFragments);
	map<UInt32,Listener*>::const_iterator it;
	for(it=_listeners.begin();it!=_listeners.end();++it) {
		it->second->pushAudioPacket(_time,packet);
		packet.reset(pos);
	}
	_handler.onAudioPacket(client,*this,_time,packet);
}

void Publication::pushVideoPacket(const Client& client,UInt32 time,PacketReader& packet,UInt32 numberLostFragments) {
	if(_publisherId==0) {
		ERROR("Video packet pushed on a publication %u who is idle",_publisherId);
		return;
	}
	
	if(time&0xFF000000) // TODO solve it!
		time = _time;
	_time = time;

	// if some lost packet, it can be a keyframe, to avoid break video, we must wait next key frame
	if(numberLostFragments>0)
		_firstKeyFrame=false;

	// is keyframe?
	if(((*packet.current())&0xF0) == 0x10)
		_firstKeyFrame = true;

	_videoQOS.add(_time,packet.fragments,numberLostFragments);
	if(numberLostFragments>0)
		INFO("%u video fragments lost on publication %u",numberLostFragments,_publisherId);

	if(!_firstKeyFrame) {
		WARN("No key frame available on publication %u, frame dropped to wait first key frame",_publisherId);
		++(UInt32&)_videoQOS.droppedFrames;
		return;
	}

	int pos = packet.position();
	map<UInt32,Listener*>::const_iterator it;
	for(it=_listeners.begin();it!=_listeners.end();++it) {
		it->second->pushVideoPacket(_time,packet);
		packet.reset(pos);
	}
	_handler.onVideoPacket(client,*this,_time,packet);
}



} // namespace Cumulus
