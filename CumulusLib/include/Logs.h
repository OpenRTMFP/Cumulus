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
#include "Logger.h"

namespace Cumulus {

class CUMULUS_API Logs
{
public:
	static void			SetLogger(Logger& logger);


#if defined(CUMULUS_EXPORTS)
	static Logger*		GetLogger();
#endif
	
	
private:
	Logs();
	~Logs();
	
	static Logger* s_pLogger;
};


inline void Logs::SetLogger(Logger& logger) {
	s_pLogger = &logger;
}


#if defined(CUMULUS_EXPORTS)

	inline Logger* Logs::GetLogger() {
		return s_pLogger;
	}

	// Empecher le traitement des chaines si de toute façon le log n'a aucun CLogReceiver!!
	#define LOG(PRIO,FILE,LINE,FMT, ...) { \
		if(Logs::GetLogger()) {\
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
