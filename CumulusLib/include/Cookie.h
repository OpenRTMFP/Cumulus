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
#include "Invoker.h"
#include "CookieComputing.h"

namespace Cumulus {

class Cookie {
	friend class Target;
public:
	Cookie(Handshake& handshake,Invoker& invoker,const std::string& tag,const std::string& queryUrl); // For normal cookie
	Cookie(Handshake& handshake,Invoker& invoker,const std::string& tag,Target& target); // For a Man-In-The-Middle peer/peer cookie
	virtual ~Cookie();

	const Poco::UInt32				id;
	const Poco::UInt32				farId;
	const std::string				tag;
	const std::string				queryUrl;

	const Poco::UInt8				peerId[ID_SIZE];
	const Poco::Net::SocketAddress	peerAddress;

	const Poco::UInt8*				value();
	std::vector<Poco::UInt8>&		initiatorKey();
	std::vector<Poco::UInt8>&		initiatorNonce();
	std::vector<Poco::UInt8>&		sharedSecret();
	const Poco::UInt8*				decryptKey();
	const Poco::UInt8*				encryptKey();
	
	void							computeKeys();
	bool							obsolete();

	Poco::UInt16					length();
	void							write();
	Poco::UInt16					read(PacketWriter& writer);

	// just for middle mode!
	Target*							pTarget;
private:
	PoolThread*						_pComputingThread;
	Poco::AutoPtr<CookieComputing>	_pCookieComputing;
	Poco::Timestamp					_createdTimestamp;

	Poco::UInt8						_buffer[256];
	PacketWriter					_writer;
	Invoker&						_invoker;
};

inline const Poco::UInt8* Cookie::value() {
	return _pCookieComputing->value;
}

inline const Poco::UInt8* Cookie::decryptKey() {
	return _pCookieComputing->decryptKey;
}

inline const Poco::UInt8* Cookie::encryptKey() {
	return _pCookieComputing->encryptKey;
}

inline void Cookie::computeKeys() {
	_pComputingThread = _invoker.poolThreads.enqueue(_pCookieComputing.cast<WorkThread>(),_pComputingThread);
}

inline Poco::UInt16	Cookie::length() {
	return _writer.length();
}

inline bool Cookie::obsolete() {
	return _createdTimestamp.isElapsed(120000000); // after 2 mn
}

inline std::vector<Poco::UInt8>& Cookie::sharedSecret() {
	return _pCookieComputing->sharedSecret;
}

inline std::vector<Poco::UInt8>& Cookie::initiatorKey() {
	return _pCookieComputing->initiatorKey;
}

inline std::vector<Poco::UInt8>& Cookie::initiatorNonce() {
	return _pCookieComputing->initiatorNonce;
}


} // namespace Cumulus
