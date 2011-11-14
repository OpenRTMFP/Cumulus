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

#include "StatusWriter.h"
#include "Handler.h"

using namespace std;
using namespace Poco;
using namespace Cumulus;

StatusWriter::StatusWriter(const string& signature,BandWriter& band) : FlowWriter(signature,band),idPublisherWithListeners(0) {
	
}

StatusWriter::~StatusWriter() {
}

void StatusWriter::init(Handler& handler) {
	Publications::Iterator it;
	for(it=handler.streams.publications.begin();it!=handler.streams.publications.end();++it) {
		if(it->second->publisherId()!=0)
			_publications.insert(pair<UInt32,Publication*>(it->second->publisherId(),it->second));
	}
	refresh(handler);
}

void StatusWriter::refresh(Handler& handler) {
	AMFWriter& writer = writeAMFMessage("status");
	writer.beginObject();
		writer.writeObjectProperty("type","refresh");
		writer.writeObjectProperty("clients",handler.clients.count());

		writer.writeObjectArrayProperty("publications",_publications.size());
		map<UInt32,const Publication* const>::const_iterator it;
		const Publication* pListenersPublication=NULL;
		for(it=_publications.begin();it!=_publications.end();++it) {
			writer.beginObject();
				writePublicationContent(*it->second,writer);
				if(idPublisherWithListeners==it->second->publisherId())
					pListenersPublication = it->second;
			writer.endObject();
		}

		if(pListenersPublication) {
			writer.writeObjectArrayProperty("listeners",pListenersPublication->listeners.count());
			Listeners::Iterator it2;
			for(it2=pListenersPublication->listeners.begin();it2!=pListenersPublication->listeners.end();++it2) {
				writer.beginObject();
					writer.writeObjectProperty("id",it2->second->id);
					writer.writeObjectProperty("vLost",it2->second->videoQOS().lostRate);
					writer.writeObjectProperty("aLost",it2->second->audioQOS().lostRate);
					writer.writeObjectProperty("vLatency",it2->second->videoQOS().latency);
					writer.writeObjectProperty("aLatency",it2->second->audioQOS().latency);
					writer.writeObjectProperty("df",it2->second->videoQOS().droppedFrames);
				writer.endObject();
			}
		}

		if(handler.edges.count()>0) {
			writer.writeObjectArrayProperty("edges",handler.edges.count());
			map<string,Edge*>::const_iterator it3;
			for(it3=handler.edges.begin();it3!=handler.edges.end();++it3) {
				writer.beginObject();
					writer.writeObjectProperty("count",it3->second->count);
					writer.writeObjectProperty("host",it3->second->address.host);
					writer.writeObjectProperty("port",it3->second->address.port);
				writer.endObject();
			}
		}

	writer.endObject();
	flush(true);
}

void StatusWriter::addPublication(const Publication& publication) {
	_publications.insert(pair<UInt32,const Publication* const>(publication.publisherId(),&publication));
	AMFWriter& writer = writeAMFMessage("status");
	writer.beginObject();
		writer.writeObjectProperty("type","add");
		writer.beginSubObject("publication");
			writePublicationContent(publication,writer);
		writer.endObject();
	writer.endObject();
	flush(true);
}

void StatusWriter::removePublication(const Publication& publication) {
	if(idPublisherWithListeners==publication.publisherId())
		idPublisherWithListeners=0;
	_publications.erase(publication.publisherId());
	AMFWriter& writer = writeAMFMessage("status");
	writer.beginObject();
		writer.writeObjectProperty("type","rem");
		writer.beginSubObject("publication");
			writer.writeObjectProperty("id",publication.publisherId());
		writer.endObject();
	writer.endObject();
	flush(true);
}

void StatusWriter::audioLostRate(const Publication& publication) {
	AMFWriter& writer = writeAMFMessage("status");
	writer.beginObject();
		writer.writeObjectProperty("type","aQOS");
		writer.beginSubObject("publication");
			writer.writeObjectProperty("id",publication.publisherId());
			writer.writeObjectProperty("aLost",publication.audioQOS().lostRate);
			writer.writeObjectProperty("aLatency",publication.audioQOS().latency);
		writer.endObject();
	writer.endObject();
	flush(true);
}
void StatusWriter::videoLostRate(const Publication& publication) {
	AMFWriter& writer = writeAMFMessage("status");
	writer.beginObject();
		writer.writeObjectProperty("type","vQOS");
		writer.beginSubObject("publication");
			writer.writeObjectProperty("id",publication.publisherId());
			writer.writeObjectProperty("vLost",publication.videoQOS().lostRate);
			writer.writeObjectProperty("vLatency",publication.videoQOS().latency);
		writer.endObject();
	writer.endObject();
	flush(true);
}

void StatusWriter::manage(Handler& handler) {
	FlowWriter::manage(handler);
	refresh(handler);
}

void StatusWriter::writePublicationContent(const Publication& publication,AMFWriter& writer) {
	writer.writeObjectProperty("id",publication.publisherId());
	writer.writeObjectProperty("name",publication.name());
	writer.writeObjectProperty("listeners",publication.listeners.count());
	writer.writeObjectProperty("vLost",publication.videoQOS().lostRate);
	writer.writeObjectProperty("aLost",publication.audioQOS().lostRate);
	writer.writeObjectProperty("vLatency",publication.videoQOS().latency);
	writer.writeObjectProperty("aLatency",publication.audioQOS().latency);
	writer.writeObjectProperty("df",publication.videoQOS().droppedFrames);
}
