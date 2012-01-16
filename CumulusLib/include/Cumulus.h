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

#if defined(STATIC) && !defined(POCO_STATIC)
	#define POCO_STATIC
#endif


#include "Poco/Foundation.h"
#include <stdio.h>


#if defined(POCO_OS_FAMILY_WINDOWS)
	#define snprintf _snprintf
#endif

//
// Automatically link Base library.
//
#if defined(_MSC_VER)
	#if !defined(POCO_NO_AUTOMATIC_LIBS)
		#if defined(_DEBUG)
			#pragma comment(lib, "CumulusLibd.lib")
			#pragma comment(lib, "libeay32MTd.lib")
			#pragma comment(lib, "ssleay32MTd.lib")
		#else
			#pragma comment(lib, "CumulusLib.lib")
			#pragma comment(lib, "libeay32MT.lib")
			#pragma comment(lib, "ssleay32MT.lib")
		#endif
	#endif
#endif

// Fonctions round

#define ROUND(val) floor( val + 0.5 )

//
// Memory Leak
//

#if defined(POCO_OS_FAMILY_WINDOWS) && defined(_DEBUG)
	#include <map> // A cause d'un pb avec le nouveau new debug!
	#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

void DetectMemoryLeak();
