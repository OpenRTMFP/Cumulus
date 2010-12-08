/* 
	Copyright 2010 cumulus.dev@gmail.com
 
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
#include "PacketReader.h"
#include "AESEngine.h"
#include <openssl/dh.h>

#define RTMFP_SYMETRIC_KEY (Poco::UInt8*)"Adobe Systems 02"

namespace Cumulus {

class RTMFP
{
public:
	static Poco::UInt32				Unpack(PacketReader& packet);
	static void						Pack(PacketWriter packet,Poco::UInt32 farId);
	static bool						Decode(AESEngine& aes,PacketReader& packet);
	static void						Encode(AESEngine& aes,PacketWriter packet);
	static bool						IsValidPacket(PacketReader packet);
	static Poco::UInt16				CheckSum(PacketReader packet);

	static DH*						BeginDiffieHellman(Poco::UInt8* pubKey);
	static void						EndDiffieHellman(DH* pDH,const Poco::UInt8* farPubKey,Poco::UInt8* sharedSecret);

	static void						ComputeAsymetricKeys(const Poco::UInt8* sharedSecret,
														 Poco::UInt8* requestKey,
														 Poco::UInt8* responseKey,
														 const Poco::UInt8* serverPubKey,
														 const std::string& serverSignature,
														 const std::string& clientCertificat);
	static Poco::UInt16				Timestamp();

private:

	RTMFP();
	~RTMFP();
};

}  // namespace Cumulus
