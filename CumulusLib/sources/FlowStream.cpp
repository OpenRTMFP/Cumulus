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

#include "FlowStream.h"
#include "Logs.h"
#include "Util.h"

using namespace std;
using namespace Poco;


namespace Cumulus {

string FlowStream::Signature("\x00\x54\x43\x04",4);
string FlowStream::_Name("NetStream");

FlowStream::FlowStream(UInt32 id,const string& signature,Peer& peer,Invoker& invoker,BandWriter& band) : Flow(id,signature,_Name,peer,invoker,band),_pPublication(NULL),_state(IDLE),_numberLostFragments(0) {
	PacketReader reader((const UInt8*)signature.c_str(),signature.length());
	reader.next(4);
	_index = reader.read7BitValue();
	Publications::Iterator it = invoker.publications(_index);
	if(it!=invoker.publications.end())
		_pPublication = it->second;
}

FlowStream::~FlowStream() {
	disengage();
}

void FlowStream::disengage() {
	// Stop the current  job
	if(_state==PUBLISHING) {
		invoker._streams.unpublish(peer,_index,name);
		writer.writeStatusResponse("Unpublish.Success",name + " is now unpublished");
	} else if(_state==PLAYING) {
		invoker._streams.unsubscribe(peer,_index,name);
		writer.writeStatusResponse("Play.Stop","Stopped playing " + name);
	}
	_state=IDLE;
}

void FlowStream::audioHandler(PacketReader& packet) {
	if(_pPublication && _pPublication->publisherId() == _index) {
		_pPublication->pushAudioPacket(packet.read32(),packet,_numberLostFragments);
		_numberLostFragments=0;
	} else
		fail("an audio packet has been received on a no publisher FlowStream");
}

void FlowStream::videoHandler(PacketReader& packet) {
	if(_pPublication && _pPublication->publisherId() == _index) {
		_pPublication->pushVideoPacket(packet.read32(),packet,_numberLostFragments);
		_numberLostFragments=0;
	} else
		fail("a video packet has been received on a no publisher FlowStream");
}

void FlowStream::commitHandler() {
	if(_pPublication && _pPublication->publisherId() == _index)
		_pPublication->flush();
}

void FlowStream::rawHandler(UInt8 type,PacketReader& data) {
	UInt16 flag = data.read16();
	if(flag==0x22) { // TODO Here we receive publication bounds (id + tracks), useless? maybe to record a file and sync tracks?
		//TRACE("Bound %u : %u %u",id,data.read32(),data.read32());
		return;
	}
	ERROR("Unknown raw flag %u on FlowStream %u",flag,id);
	Flow::rawHandler(type,data);
}

void FlowStream::lostFragmentsHandler(UInt32 count) {
	if(_pPublication)
		_numberLostFragments += count;
	Flow::lostFragmentsHandler(count); // TODO remove it in a "no reliable" case? To avoid a too much WARN log written?
}

void FlowStream::messageHandler(const string& action,AMFReader& message) {

	if(action=="play") {
		disengage();

		message.read((string&)name);
		// TODO implements completly NetStream.play method, with possible NetStream.play.failed too!
		double start = -2000;
		if(message.available())
			start = message.readNumber();

		if(invoker._streams.subscribe(peer,_index,name,writer,start))
			_state = PLAYING;

	} else if(action == "closeStream") {
		disengage();
	} else if(action=="publish") {

		disengage();

		string type;
		message.read((string&)name);
		if(message.available())
			message.read(type); // TODO recording publication feature!

		try {
			invoker._streams.publish(peer,_index,name);
			_state = PUBLISHING;
			writer.writeStatusResponse("Publish.Start",name +" is now published");
		} catch(Exception& ex) {
			writer.writeStatusResponse(ex.code() == Publication::BADNAME ? "Publish.BadName" : "Publish.Failed", ex.displayText());
		}

	} else if(_state==PUBLISHING) {
		if(!_pPublication) {
			Publications::Iterator it = invoker.publications(name);
			if(it!=invoker.publications.end())
				_pPublication = it->second;
			else
				ERROR("Publication %s unfound, related for the %s message",name.c_str(),action.c_str());
		}
		if(_pPublication)
			_pPublication->pushDataPacket(action,message.reader);
	} else
		Flow::messageHandler(action,message);

}

} // namespace Cumulus
