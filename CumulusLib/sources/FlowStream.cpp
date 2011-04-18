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

using namespace std;
using namespace Poco;


namespace Cumulus {

string FlowStream::s_signature("\x00\x54\x43\x04",4);
string FlowStream::s_name("NetStream");

FlowStream::FlowStream(UInt8 id,const string& signature,Peer& peer,Session& session,ServerHandler& serverHandler) : Flow(id,_signature,s_name,peer,session,serverHandler),_signature(signature),_pSubscription(NULL),_state(IDLE) {
	PacketReader reader((const UInt8*)signature.c_str(),signature.length());
	reader.next(4);
	_index = reader.read7BitValue();
	_pSubscription = serverHandler.streams.subscription(_index);
}

FlowStream::~FlowStream() {
}

void FlowStream::audioHandler(PacketReader& packet) {
	if(_pSubscription && _pSubscription->idPublisher == _index)
		_pSubscription->pushAudioPacket(packet);
	else
		fail();
}

void FlowStream::videoHandler(PacketReader& packet) {
	if(_pSubscription && _pSubscription->idPublisher == _index)
		_pSubscription->pushVideoPacket(packet);
	else
		fail();
}

void FlowStream::rawHandler(UInt8 type,PacketReader& data) {
	if(_pSubscription)
		_pSubscription->pushRawPacket(type,data);
	else
		Flow::rawHandler(type,data);
}

void FlowStream::complete() {
	Flow::complete();
	// Stop the current  job
	if(_state==PUBLISHING)
		serverHandler.streams.unpublish(_index,_name);
	 else if(_state==PLAYING)
		serverHandler.streams.unsubscribe(_name,*this);
	_state=IDLE;
}

void FlowStream::messageHandler(const string& name,AMFReader& message) {

	if(name=="publish") {
		// Stop a precedent publishment
		if(_state==PUBLISHING)
			serverHandler.streams.unpublish(_index,_name);
		_state = IDLE;

		// TODO add a failed scenario?
		string type;
		message.read(_name);
		if(message.available())
			message.read(type); // TODO record!

		if(serverHandler.streams.publish(_index,_name)) {
			writeStatusResponse("Start","'" + _name +"' is now published");
			_state = PUBLISHING;
		} else
			writeErrorResponse("'" + _name +"' is already publishing","BadName");

	} else if(name=="play") {
		// Stop a precedent playing
		if(_state==PLAYING)
			serverHandler.streams.unsubscribe(_name,*this);
		_state = PLAYING;

		// TODO add a failed scenario?
		message.read(_name);
		writeStatusResponse("Start","Started playing '" + _name +"'");
		serverHandler.streams.subscribe(_name,*this);
	} else if(name == "closeStream") {
		// Stop the current  job
		if(_state==PUBLISHING) {
			serverHandler.streams.unpublish(_index,_name);
			writeSuccessResponse("Stopped publishing '" + _name +"'"); // TODO doesn't work!
		} else if(_state==PLAYING) {
			serverHandler.streams.unsubscribe(_name,*this);
			writeStatusResponse("Stop","Stopped playing '" + _name +"'"); // TODO doesn't work!
		}
		_state=IDLE;
	} else
		Flow::messageHandler(name,message);

}

} // namespace Cumulus
