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
#include "Target.h"

namespace Cumulus {

#define COOKIE_SIZE 0x40

class Cookie {
	friend class Target;
public:
	Cookie(const std::string& tag,const std::string& queryUrl); // For normal cookie
	Cookie(const std::string& tag,Target& target); // For a Man-In-The-Middle peer/peer cookie
	virtual ~Cookie();

	const std::string				tag;
	const Poco::UInt8				value[COOKIE_SIZE];
	const std::string				queryUrl;
	const Poco::UInt32				id;
	const Poco::UInt8				response;
	
	void							computeKeys(const Poco::UInt8* initiatorKey,Poco::UInt16 initKeySize,const Poco::UInt8* initiatorNonce,Poco::UInt16 initNonceSize,Poco::UInt8* decryptKey,Poco::UInt8* encryptKey);
	bool							obsolete();

	void							write();
	Poco::UInt16					read(PacketWriter& writer);

	// just for middle mode!
	Target*							pTarget;
private:
	std::vector<Poco::UInt8>		_nonce;
	Poco::Timestamp					_createdTimestamp;
	DH*								_pDH;

	Poco::UInt8						_buffer[256];
	PacketWriter					_writer;
};

inline bool Cookie::obsolete() {
	return _createdTimestamp.isElapsed(120000000); // after 2 mn
}


} // namespace Cumulus
