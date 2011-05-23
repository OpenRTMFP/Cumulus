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

FlowStream::FlowStream(UInt32 id,const string& signature,Peer& peer,ServerHandler& serverHandler,BandWriter& band) : Flow(id,signature,s_name,peer,serverHandler,band),_pPublication(NULL),_pListener(NULL),_state(IDLE) {
	PacketReader reader((const UInt8*)signature.c_str(),signature.length());
	reader.next(4);
	_index = reader.read7BitValue();
	DEBUG("Index stream : %d",_index);
	_pPublication = serverHandler.streams.publication(_index);
}

FlowStream::~FlowStream() {
}

void FlowStream::audioHandler(PacketReader& packet) {
	if(_pPublication && _pPublication->publisherId == _index)
		_pPublication->pushAudioPacket(packet);
	else
		fail("an audio packet has been received on a no subscriber FlowStream");
}

void FlowStream::videoHandler(PacketReader& packet) {
	if(_pPublication && _pPublication->publisherId == _index)
		_pPublication->pushVideoPacket(packet);
	else
		fail("a video packet has been received on a no subscriber FlowStream");
}

void FlowStream::rawHandler(UInt8 type,PacketReader& data) {
//	if(_pPublication)
//		_pPublication->pushRawPacket(type,data);
//	else
		Flow::rawHandler(type,data);
}

void FlowStream::complete() {
	// Stop the current  job
	if(_state==PUBLISHING)
		serverHandler.streams.unpublish(_index,name);
	else if(_state==PLAYING) {
		serverHandler.streams.unsubscribe(name,*_pListener);
		_pListener->close();
	}
	_state=IDLE;
	Flow::complete();
}

void FlowStream::messageHandler(const string& action,AMFReader& message) {

	if(action=="|RtmpSampleAccess") {
		// TODO?
		bool value1 = message.readBool();
		bool value2 = message.readBool();
	} else if(action=="play") {
		// Stop a precedent playing
		if(_state==PLAYING) {
			serverHandler.streams.unsubscribe(name,*_pListener);
			_pListener->close();
		}
		_state = PLAYING;

		// TODO add a failed scenario?
		message.read((string&)name);

		BinaryWriter& writer1 = writer.writeRawMessage(true);
		writer1.write8(0x0F);
		writer1.write32(0x00);
		writer1.write8(0x00);
		AMFWriter amf(writer1);
		amf.write("|RtmpSampleAccess");
		amf.writeBool(false);
		amf.writeBool(false);

	/*	BinaryWriter& writer2 = writeRawMessage();
		writer2.write16(0x00);
		writer2.write32(0x02);*/

		writer.writeStatusResponse("Reset","Playing and resetting '" + name +"'");

		writer.writeStatusResponse("Start","Started playing '" + name +"'");

		/*BinaryWriter& writer3 = writer.writeRawMessage(); TODO added? useful?
		writer3.write16(0x22);
		writer3.write32(0);
		writer3.write32(0x02);*/

		_pListener = &newFlowWriter<Listener>(writer.signature);
		serverHandler.streams.subscribe(name,*_pListener);


	} else if(action == "closeStream") {
		// Stop the current  job
		if(_state==PUBLISHING) {
			serverHandler.streams.unpublish(_index,name);
			writer.writeSuccessResponse("Stopped publishing '" + name +"'"); // TODO doesn't work! NetStream.Unpublish.Success should be!
		} else if(_state==PLAYING) {
			serverHandler.streams.unsubscribe(name,*_pListener);
			_pListener->close();
			writer.writeStatusResponse("Stop","Stopped playing '" + name +"'"); // TODO doesn't work!
		}
		_state=IDLE;
	} else if(action=="publish") {

		// Stop a precedent publishment
		if(_state==PUBLISHING)
			serverHandler.streams.unpublish(_index,name);
		_state = IDLE;

		string type;
		message.read((string&)name);
		if(message.available())
			message.read(type); // TODO record!

		// TODO add a failed scenario?

		if(serverHandler.streams.publish(_index,name)) {
			writer.writeStatusResponse("Start","'" + name +"' is now published");
			_state = PUBLISHING;
		} else
			writer.writeErrorResponse("'" + name +"' is already publishing","BadName");

	} else
		Flow::messageHandler(action,message);

}

} // namespace Cumulus
