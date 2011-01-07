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
#include "PacketReader.h"
#include "AESEngine.h"
#include "Poco/Timestamp.h"
#include <openssl/dh.h>

namespace Cumulus {

#define RTMFP_SYMETRIC_KEY (Poco::UInt8*)"Adobe Systems 02"
#define RTMFP_DEFAULT_PORT 1935

class RTMFP
{
public:
	static Poco::UInt32				Unpack(PacketReader& packet);
	static void						Pack(PacketWriter packet,Poco::UInt32 farId=0);

	static bool						Decode(PacketReader& packet);
	static bool						Decode(AESEngine& aesDecrypt,PacketReader& packet);
	static void						Encode(PacketWriter packet);
	static void						Encode(AESEngine& aesEncrypt,PacketWriter packet);
	
	static bool						IsValidPacket(PacketReader& packet);
	static Poco::UInt16				CheckSum(PacketReader packet);

	static DH*						BeginDiffieHellman(Poco::UInt8* pubKey);
	static void						EndDiffieHellman(DH* pDH,const Poco::UInt8* farPubKey,Poco::UInt8* sharedSecret);

	static void						ComputeAsymetricKeys(const Poco::UInt8* sharedSecret,
														 const Poco::UInt8* serverPubKey,
														 const std::string& serverSignature,
														 const std::string& clientCertificat,
														 Poco::UInt8* requestKey,
														 Poco::UInt8* responseKey);
	static Poco::UInt16				TimeNow();
	static Poco::UInt16				Time(Poco::Timestamp::TimeVal timeVal);

private:

	static AESEngine				s_aesDecrypt;
	static AESEngine				s_aesEncrypt;

	RTMFP();
	~RTMFP();
};

inline bool RTMFP::Decode(PacketReader& packet) {
	return Decode(s_aesDecrypt,packet);
}

inline void RTMFP::Encode(PacketWriter packet) {
	Encode(s_aesEncrypt,packet);
}

inline Poco::UInt16 RTMFP::TimeNow() {
	return Time(Poco::Timestamp().epochMicroseconds());
}

}  // namespace Cumulus
