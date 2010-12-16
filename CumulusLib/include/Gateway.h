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
#include "PacketWriter.h"
#include "Poco/Net/SocketAddress.h"

namespace Cumulus {


class Gateway
{
public:
	Gateway();
	virtual ~Gateway();

	virtual Poco::UInt8 p2pHandshake(const std::string& tag,PacketWriter& response,const BLOB& peerWantedId,const Poco::Net::SocketAddress& peerAddress)=0;
	virtual Poco::UInt32 createSession(Poco::UInt32 farId,const Poco::UInt8* peerId,const Poco::Net::SocketAddress& peerAddress,const std::string& url,const Poco::UInt8* decryptKey,const Poco::UInt8* encryptKey)=0;
};


} // namespace Cumulus
