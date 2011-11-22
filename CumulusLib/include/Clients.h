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
#include "Entity.h"
#include "FlowWriter.h"
#include "Poco/URI.h"
#include "string.h"
#include <map>

namespace Cumulus {



class BandWriterNull : public BandWriter {
public:
	BandWriterNull() {}
	virtual ~BandWriterNull() {}

	void			initFlowWriter(FlowWriter& flowWriter){}
	void			resetFlowWriter(FlowWriter& flowWriter){}

	bool			failed() const{return true;}
	bool			canWriteFollowing(FlowWriter& flowWriter){return false;}
	PacketWriter&	writer(){return WriterNull;}
	PacketWriter&	writeMessage(Poco::UInt8 type,Poco::UInt16 length,FlowWriter* pFlowWriter=NULL){return WriterNull;}
	void			flush(bool echoTime=true){}
private:
	static PacketWriter WriterNull;
	
};


class Client : public Entity,public Poco::Util::MapConfiguration {
public:
	Client();
	Client(const Client& client);
	virtual ~Client();

	const std::string							path;
	const Poco::URI								swfUrl;
	const Poco::URI								pageUrl;

	FlowWriter&									writer();			
protected:
	FlowWriter*									_pFlowWriter;
private:
	void										copyProperties(const Poco::Util::AbstractConfiguration& abstractConfigs,const std::string& root="");
	static FlowWriter							_FlowWriterNull;
	static BandWriterNull						_BandWriterNull;
};


class Clients {
public:
	struct Compare {
	   bool operator()(const Poco::UInt8* a,const Poco::UInt8* b) const {
		   return memcmp(a,b,ID_SIZE)<0;
	   }
	};

	Clients(std::map<const Poco::UInt8*,Client*,Compare>& clients);
	~Clients();

	typedef std::map<const Poco::UInt8*,Client*,Compare>::iterator Iterator;

	Iterator		begin();
	Iterator		end();
	Poco::UInt32	count();

	Client* operator()(const Poco::UInt8* id);
private:

	std::map<const Poco::UInt8*,Client*,Compare>&	_clients;
};

inline Poco::UInt32 Clients::count() {
	return _clients.size();
}

inline Clients::Iterator Clients::begin() {
	return _clients.begin();
}

inline Clients::Iterator Clients::end() {
	return _clients.end();
}



} // namespace Cumulus
