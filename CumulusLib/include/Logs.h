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
#include "Logger.h"
#include "PacketReader.h"
#include "PacketWriter.h"

#ifdef CUMULUS_EXPORTS
	#define CUMULUS_LOGS
#endif

namespace Cumulus {

class CUMULUS_API Logs
{
public:
	static void			SetLogger(Logger& logger);
	static void			SetLevel(Poco::UInt8 level);
	static void			EnableDump(bool all=false);
	static void			DisableDump();


#ifdef CUMULUS_LOGS
	static Logger*				GetLogger();
	static Poco::UInt8			GetLevel();
	static void					Dump(const Poco::UInt8* data,int size,const char* header=NULL,bool required=true);
	static void					Dump(PacketReader& packet,const char* header=NULL,bool required=true);
	static void					Dump(PacketWriter& packet,const char* header=NULL,bool required=true);
	static void					Dump(PacketWriter& packet,Poco::UInt16 offset,const char* header=NULL,bool required=true);
#endif
	
	
private:
	Logs();
	~Logs();
	
	static Logger*		s_pLogger;
	static bool			s_dump;
	static bool			s_dumpAll;
	static Poco::UInt8  s_level;
};

inline void Logs::DisableDump() {
	s_dump=false;
}

inline void Logs::SetLevel(Poco::UInt8 level) {
	s_level = level;
}

inline void Logs::SetLogger(Logger& logger) {
	s_pLogger = &logger;
}


#ifdef CUMULUS_LOGS

	inline void Logs::Dump(PacketReader& packet,const char* header,bool required) {
		Dump(packet.current(),packet.available(),header,required);
	}
	inline void Logs::Dump(PacketWriter& packet,const char* header,bool required) {
		Dump(packet.begin(),packet.length(),header,required);
	}
	inline void Logs::Dump(PacketWriter& packet,Poco::UInt16 offset,const char* header,bool required) {
		Dump(packet.begin()+offset,packet.length()-offset,header,required);
	}

	inline Poco::UInt8 Logs::GetLevel() {
		return s_level;
	}

	inline Logger* Logs::GetLogger() {
		return s_pLogger;
	}

	// Empecher le traitement des chaines si de toute façon le log n'a aucun CLogReceiver!
	// Ou si le level est plus détaillé que le loglevel
	#define LOG(PRIO,FILE,LINE,FMT, ...) { \
		if(Logs::GetLogger() && Logs::GetLevel()>=PRIO) {\
			char szzs[700];\
			snprintf(szzs,sizeof(szzs),FMT,## __VA_ARGS__);\
			szzs[sizeof(szzs)-1] = '\0'; \
			Logs::GetLogger()->logHandler(Poco::Thread::currentTid(),GetThreadName(),PRIO,FILE,LINE,szzs); \
		} \
	}

	#undef ERROR
	#undef DEBUG
	#undef TRACE
	#define FATAL(FMT, ...) LOG(Logger::PRIO_FATAL,__FILE__,__LINE__,FMT, ## __VA_ARGS__)
	#define CRITIC(FMT, ...) LOG(Logger::PRIO_CRITIC,__FILE__,__LINE__,FMT, ## __VA_ARGS__)
	#define ERROR(FMT, ...) LOG(Logger::PRIO_ERROR,__FILE__,__LINE__,FMT, ## __VA_ARGS__)
	#define WARN(FMT, ...) LOG(Logger::PRIO_WARN,__FILE__,__LINE__,FMT, ## __VA_ARGS__)
	#define NOTE(FMT, ...) LOG(Logger::PRIO_NOTE,__FILE__,__LINE__,FMT, ## __VA_ARGS__)
	#define INFO(FMT, ...) LOG(Logger::PRIO_INFO,__FILE__,__LINE__,FMT, ## __VA_ARGS__)
	#define DEBUG(FMT, ...) LOG(Logger::PRIO_DEBUG,__FILE__,__LINE__,FMT, ## __VA_ARGS__)
	#define TRACE(FMT, ...) LOG(Logger::PRIO_TRACE,__FILE__,__LINE__,FMT, ## __VA_ARGS__)

#endif

} // namespace Cumulus
