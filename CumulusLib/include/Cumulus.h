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


// CONSTANTES
#define MAX_SIZE_MSG 8192

#if defined(STATIC) && !defined(POCO_STATIC)
	#define POCO_STATIC
#endif


#include "Poco/Foundation.h"
#include <stdio.h>


#if defined(_WIN32)

	#define snprintf _snprintf

	#if defined(POCO_DLL)
		#if defined(CUMULUS_EXPORTS)
			#define CUMULUS_API __declspec(dllexport)
		#else
			#define CUMULUS_API __declspec(dllimport)
		#endif
	#endif

#endif

#if !defined(CUMULUS_API)
	#define CUMULUS_API
#endif

//
// Automatically link Base library.
//
#if defined(_MSC_VER)
	#if !defined(POCO_NO_AUTOMATIC_LIBS)
		#if defined(POCO_DLL)
			#if defined(_DEBUG)
				#ifndef CUMULUS_EXPORTS
					#pragma comment(lib, "CumulusLibd.lib")
				#endif
				#pragma comment(lib, "libeay32MDd.lib")
				#pragma comment(lib, "ssleay32MDd.lib")
			#else
				#ifndef CUMULUS_EXPORTS
					#pragma comment(lib, "CumulusLib.lib")
				#endif
				#pragma comment(lib, "libeay32MD.lib")
				#pragma comment(lib, "ssleay32MD.lib")
			#endif
		#else if !defined(CUMULUS_EXPORTS)
			#if defined(_DEBUG)
				#pragma comment(lib, "CumulusLibmtd.lib")
				#pragma comment(lib, "libeay32MTd.lib")
				#pragma comment(lib, "ssleay32MTd.lib")
			#else
				#pragma comment(lib, "CumulusLibmt.lib")
				#pragma comment(lib, "libeay32MT.lib")
				#pragma comment(lib, "ssleay32MT.lib")
			#endif
		#endif
	#endif
#endif

// Fonctions round

#define ROUND(val) floor( val + 0.5 )

//
// Memory Leak
//
void		SetThreadName(const char* szThreadName);
std::string GetThreadName();

#if defined(_WIN32) && defined(_DEBUG)
	#include <map> // A cause d'un pb avec le nouveau new debug!
	#include <xlocnum> // Pourquoi? je n'en sais rien
	#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
	void CUMULUS_API setFilterDebugHook(void);
#endif



