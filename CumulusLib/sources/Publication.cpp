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
#include "Poco/StreamCopier.h"
#include "string.h"

using namespace std;
using namespace Poco;


namespace Cumulus {

Publication::Publication(const string& name):_publisherId(0),_name(name),_firstKeyFrame(false),listeners(_listeners),_pPublisher(NULL),_pController(NULL) {
	DEBUG("New publication %s",_name.c_str());
}

Publication::~Publication() {
	// delete _listeners!
	map<UInt32,Listener*>::iterator it;
	for(it=_listeners.begin();it!=_listeners.end();++it)
		delete it->second;

	DEBUG("Publication %s deleted",_name.c_str());
}


bool Publication::addListener(Peer& peer,UInt32 id,FlowWriter& writer,bool unbuffered) {
	map<UInt32,Listener*>::iterator it = _listeners.lower_bound(id);
	if(it!=_listeners.end() && it->first==id) {
		WARN("Listener %u is already subscribed for publication %u",id,_publisherId);
		return true;
	}
	if(it!=_listeners.begin())
		--it;
	Listener* pListener = new Listener(id,*this,writer,unbuffered);
	string error;
	if(peer.onSubscribe(*pListener,error)) {
		_listeners.insert(it,pair<UInt32,Listener*>(id,pListener));
		writer.writeStatusResponse("Play.Reset","Playing and resetting " + _name);
		writer.writeStatusResponse("Play.Start","Started playing " + _name);
		pListener->init();
		return true;
	}
	writer.writeStatusResponse("Play.Failed",error.empty() ? ("Not authorized to play " + _name) : error);
	delete pListener;
	return false;
}

void Publication::removeListener(Peer& peer,UInt32 id) {
	map<UInt32,Listener*>::iterator it = _listeners.find(id);
	if(it==_listeners.end()) {
		WARN("Listener %u is already unsubscribed of publication %u",id,_publisherId);
		return;
	}
	Listener* pListener = it->second;
	peer.onUnsubscribe(*pListener);
	_listeners.erase(it);
	delete pListener;
}

void Publication::closePublisher(const std::string& code,const std::string& description) {
	if(_publisherId==0) {
		ERROR("Publication %s is not published",_name.c_str());
		return;
	}
	if(_pController) {
		if(!code.empty())
			_pController->writeStatusResponse(code,description);
		_pController->writeAMFMessage("close");
	} else
		WARN("Publisher %u has no controller to close it",_publisherId);

}

void Publication::start(Peer& peer,UInt32 publisherId,FlowWriter* pController) {
	if(_publisherId!=0) {
		// has already a publisher
		if(pController)
			pController->writeStatusResponse("Publish.BadName",_name + " is already published");
		throw Exception(_name + " is already published");
	}
	_publisherId = publisherId;
	string error;
	if(!peer.onPublish(*this,error)) {
		if(error.empty())
			error = "Not allowed to publish " + _name;
		_publisherId=0;
		if(pController)
			pController->writeStatusResponse("Publish.BadName",error);
		throw Exception(error);
	}
	_pPublisher=&peer;
	_pController=pController;
	_firstKeyFrame=false;
	map<UInt32,Listener*>::const_iterator it;
	for(it=_listeners.begin();it!=_listeners.end();++it)
		it->second->startPublishing(_name);
	flush();
	if(pController)
		pController->writeStatusResponse("Publish.Start",_name +" is now published");
}

void Publication::stop(Peer& peer,UInt32 publisherId) {
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
	peer.onUnpublish(*this);
	_videoQOS.reset();
	_audioQOS.reset();
	_publisherId = 0;
	_pPublisher=NULL;
	_pController=NULL;
	return;
}

void Publication::flush() {
	map<UInt32,Listener*>::const_iterator it;
	for(it=_listeners.begin();it!=_listeners.end();++it)
		it->second->flush();
}

void Publication::pushDataPacket(const string& name,PacketReader& packet) {
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
	_pPublisher->onDataPacket(*this,name,packet);
}

void Publication::pushAudioPacket(UInt32 time,PacketReader& packet,UInt32 numberLostFragments) {
	if(_publisherId==0) {
		ERROR("Audio packet pushed on a publication %u who is idle",_publisherId);
		return;
	}

	int pos = packet.position();
	if(numberLostFragments>0)
		INFO("%u audio fragments lost on publication %u",numberLostFragments,_publisherId);
	_audioQOS.add(time,packet.fragments,numberLostFragments,packet.available()+5);
	map<UInt32,Listener*>::const_iterator it;
	for(it=_listeners.begin();it!=_listeners.end();++it) {
		it->second->pushAudioPacket(time,packet);
		packet.reset(pos);
	}
	_pPublisher->onAudioPacket(*this,time,packet);
}

void Publication::pushVideoPacket(UInt32 time,PacketReader& packet,UInt32 numberLostFragments) {
	if(_publisherId==0) {
		ERROR("Video packet pushed on a publication %u who is idle",_publisherId);
		return;
	}

	// if some lost packet, it can be a keyframe, to avoid break video, we must wait next key frame
	if(numberLostFragments>0)
		_firstKeyFrame=false;

	// is keyframe?
	if(((*packet.current())&0xF0) == 0x10)
		_firstKeyFrame = true;

	_videoQOS.add(time,packet.fragments,numberLostFragments,packet.available()+5);
	if(numberLostFragments>0)
		INFO("%u video fragments lost on publication %u",numberLostFragments,_publisherId);

	if(!_firstKeyFrame) {
		DEBUG("No key frame available on publication %u, frame dropped to wait first key frame",_publisherId);
		++(UInt32&)_videoQOS.droppedFrames;
		return;
	}

	int pos = packet.position();
	map<UInt32,Listener*>::const_iterator it;
	for(it=_listeners.begin();it!=_listeners.end();++it) {
		it->second->pushVideoPacket(time,packet);
		packet.reset(pos);
	}
	_pPublisher->onVideoPacket(*this,time,packet);
}



} // namespace Cumulus
