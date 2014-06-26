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

namespace Cumulus {


class BandWriterNull : public BandWriter {
public:
	BandWriterNull() {}
	virtual ~BandWriterNull() {}

	void			initFlowWriter(FlowWriter& flowWriter){}
	void			close(){}
	void			resetFlowWriter(FlowWriter& flowWriter){}
	bool			failed() const{return true;}
	bool			canWriteFollowing(FlowWriter& flowWriter){return false;}
	PacketWriter&	writer(){return WriterNull;}
	PacketWriter&	writeMessage(Poco::UInt8 type,Poco::UInt16 length,FlowWriter* pFlowWriter=NULL){return WriterNull;}
	void			flush(bool echoTime=true,AESEngine::Type type=AESEngine::DEFAULT){}
private:
	static PacketWriter WriterNull;
	
};


class Client : public Entity {
public:
	Client();
	virtual ~Client();

	const Poco::Net::SocketAddress				address;
	const std::string							path;
	const std::map<std::string,std::string>		properties;
	const Poco::URI								swfUrl;
	const Poco::URI								pageUrl;
	const std::string							flashVersion;
	const Poco::UInt16							ping;
	const Poco::Timestamp						lastReceptionTime;

	template<class ObjectType>
	void pinObject(ObjectType& object) {
		_pObject = (void*)&object;
	}
	template<class ObjectType>
	ObjectType* object() const {
		return (ObjectType*)_pObject;
	}

	FlowWriter&									writer();			
protected:
	FlowWriter*									_pFlowWriter;
private:
	void*										_pObject;
	static FlowWriter							_FlowWriterNull;
	static BandWriterNull						_BandWriterNull;
};

inline FlowWriter&	Client::writer() {
	return _pFlowWriter ? *_pFlowWriter : _FlowWriterNull;
}


} // namespace Cumulus
