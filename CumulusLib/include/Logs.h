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

namespace Cumulus {

class CUMULUS_API Logs
{
public:
	static void			SetLogger(Logger& logger);
	static void			SetLevel(Poco::UInt8 level);
	static void			Dump(bool activate,const std::string& file="");
	static void			Middle(bool activate);


#if defined(CUMULUS_EXPORTS)
	static Logger*				GetLogger();
	static bool					Dump();
	static bool					Middle();
	static const std::string&	DumpFile();
	static Poco::UInt8			Level();
#endif
	
	
private:
	Logs();
	~Logs();
	
	static Logger*	s_pLogger;

	static bool			s_dump;
	static bool			s_middle;
	static std::string	s_file;
	static Poco::UInt8  s_level;
};


inline void Logs::SetLogger(Logger& logger) {
	s_pLogger = &logger;
}

inline void Logs::Middle(bool activate) {
	s_middle = activate;
}


#if defined(CUMULUS_EXPORTS)

	inline bool Logs::Dump() {
		return s_dump;
	}

	inline bool Logs::Middle() {
		return s_middle;
	}

	inline Poco::UInt8 Logs::Level() {
		return s_level;
	}

	inline const std::string& Logs::DumpFile() {
		return s_file;
	}

	inline Logger* Logs::GetLogger() {
		return s_pLogger;
	}

	// Empecher le traitement des chaines si de toute façon le log n'a aucun CLogReceiver!
	// Ou si le level est plus détaillé que le loglevel
	#define LOG(PRIO,FILE,LINE,FMT, ...) { \
		if(Logs::GetLogger() && Logs::Level()>=PRIO) {\
			char szzs[700];\
			snprintf(szzs,sizeof(szzs),FMT,## __VA_ARGS__);\
			szzs[sizeof(szzs)-1] = '\0'; \
			Logs::GetLogger()->logHandler(Thread::currentTid(),GetThreadName(),PRIO,FILE,LINE,szzs); \
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
