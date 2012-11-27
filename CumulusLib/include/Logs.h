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

namespace Cumulus {

class Logs
{
public:
	enum DumpMode {
		NOTHING			= 0,
		EXTERNAL        = 1,
		MIDDLE			= 2,
		ALL				= 3
	};

	static void			SetLogger(Logger& logger);
	static void			SetLevel(Poco::UInt8 level);
	static void			SetDump(DumpMode mode);

#ifdef CUMULUS_LOGS
	static DumpMode				GetDump();
	static Logger*				GetLogger();
	static Poco::UInt8			GetLevel();
	static void					Dump(const Poco::UInt8* data,Poco::UInt32 size,const char* header=NULL);
	static void					Dump(PacketReader& packet,const char* header=NULL);
	static void					Dump(PacketWriter& packet,const char* header=NULL);
	static void					Dump(PacketWriter& packet,Poco::UInt16 offset,const char* header=NULL);
#endif
	
	
private:
	Logs();
	~Logs();
	
	static Logger*		_PLogger;
	static DumpMode		_DumpMode;
	static Poco::UInt8  _Level;
};

inline void Logs::SetDump(DumpMode mode) {
	_DumpMode=mode;
}

inline void Logs::SetLevel(Poco::UInt8 level) {
	_Level = level;
}

inline void Logs::SetLogger(Logger& logger) {
	_PLogger = &logger;
}


#ifdef CUMULUS_LOGS

	inline Logs::DumpMode Logs::GetDump() {
		return _DumpMode;
	}

	inline void Logs::Dump(PacketReader& packet,const char* header) {
		Dump(packet.current(),packet.available(),header);
	}
	inline void Logs::Dump(PacketWriter& packet,const char* header) {
		Dump(packet.begin(),packet.length(),header);
	}
	inline void Logs::Dump(PacketWriter& packet,Poco::UInt16 offset,const char* header) {
		Dump(packet.begin()+offset,packet.length()-offset,header);
	}

	inline Poco::UInt8 Logs::GetLevel() {
		return _Level;
	}

	inline Logger* Logs::GetLogger() {
		return _PLogger;
	}

	// Empecher le traitement des chaines si de toute façon le log n'a aucun CLogReceiver!
	// Ou si le level est plus détaillé que le loglevel
	#define LOG(PRIO,FILE,LINE,FMT, ...) { \
		if(Cumulus::Logs::GetLogger() && Cumulus::Logs::GetLevel()>=PRIO) {\
			char szzs[700];\
			snprintf(szzs,sizeof(szzs),FMT,## __VA_ARGS__);\
			szzs[sizeof(szzs)-1] = '\0'; \
			Cumulus::Logs::GetLogger()->logHandler(Poco::Thread::currentTid(),Poco::Thread::current() ? Poco::Thread::current()->name() : "",PRIO,FILE,LINE,szzs); \
		} \
	}

	#undef ERROR
	#undef DEBUG
	#undef TRACE
	#define FATAL(FMT, ...) LOG(Cumulus::Logger::PRIO_FATAL,__FILE__,__LINE__,FMT, ## __VA_ARGS__)
	#define CRITIC(FMT, ...) LOG(Cumulus::Logger::PRIO_CRITIC,__FILE__,__LINE__,FMT, ## __VA_ARGS__)
	#define ERROR(FMT, ...) LOG(Cumulus::Logger::PRIO_ERROR,__FILE__,__LINE__,FMT, ## __VA_ARGS__)
	#define WARN(FMT, ...) LOG(Cumulus::Logger::PRIO_WARN,__FILE__,__LINE__,FMT, ## __VA_ARGS__)
	#define NOTE(FMT, ...) LOG(Cumulus::Logger::PRIO_NOTE,__FILE__,__LINE__,FMT, ## __VA_ARGS__)
	#define INFO(FMT, ...) LOG(Cumulus::Logger::PRIO_INFO,__FILE__,__LINE__,FMT, ## __VA_ARGS__)
	#define DEBUG(FMT, ...) LOG(Cumulus::Logger::PRIO_DEBUG,__FILE__,__LINE__,FMT, ## __VA_ARGS__)
	#define TRACE(FMT, ...) LOG(Cumulus::Logger::PRIO_TRACE,__FILE__,__LINE__,FMT, ## __VA_ARGS__)

	#define DUMP_MIDDLE(...) { if(Cumulus::Logs::GetLogger() && (Cumulus::Logs::GetDump()&Cumulus::Logs::MIDDLE)) {Cumulus::Logs::Dump(__VA_ARGS__);} }
	#define DUMP(...) { if(Cumulus::Logs::GetLogger() && (Cumulus::Logs::GetDump()&Cumulus::Logs::EXTERNAL)) {Cumulus::Logs::Dump(__VA_ARGS__);} }

#endif

} // namespace Cumulus
