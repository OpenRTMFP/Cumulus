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

#include "Cumulus.h"
#include "Poco/Thread.h"

std::string MainThreadName;

#if defined(_WIN32) && defined(_DEBUG)
#include "crtdbg.h"
#include "windows.h"

 
#define FALSE   0
#define TRUE    1
 
_CRT_REPORT_HOOK prevHook;
 
int reportingHook(int reportType, char* userMessage, int* retVal)
{
  // This function is called several times for each memory leak.
  // Each time a part of the error message is supplied.
  // This holds number of subsequent detail messages after
  // a leak was reported
  const int numFollowupDebugMsgParts = 2;
  static bool ignoreMessage = false;
  static int debugMsgPartsCount = numFollowupDebugMsgParts;
  static bool firstMessage = true;

  if( strncmp(userMessage,"Detected memory leaks!\n", 10)==0) {
	ignoreMessage = true;
	return TRUE;
  } else if(strncmp(userMessage,"Dumping objects ->\n", 10)==0) {
	  return TRUE;
  } else if (ignoreMessage) {

    // check if the memory leak reporting ends
    if (strncmp(userMessage,"Object dump complete.\n", 10) == 0) {
		  _CrtSetReportHook(prevHook);
		  ignoreMessage = false;
    }

	if(debugMsgPartsCount++ < numFollowupDebugMsgParts)
		// give it back to _CrtDbgReport() to be printed to the console
		return FALSE;

 
    // something from our own code?
	if(strstr(userMessage, ".cpp") || strstr(userMessage, ".h") || strstr(userMessage, ".c")) {
		if(firstMessage) {
			OutputDebugStringA("Detected memory leaks!\nDumping objects ->\n");
			firstMessage = false;
		}
		
		debugMsgPartsCount = 0;
      // give it back to _CrtDbgReport() to be printed to the console
       return FALSE;
    } else
		return TRUE;

 } else
    // give it back to _CrtDbgReport() to be printed to the console
    return FALSE;

  
};
 
void setFilterDebugHook(void)
{
  //change the report function to only report memory leaks from program code
  prevHook = _CrtSetReportHook(reportingHook);
}

	
/// DOCME
#define MS_VC_EXCEPTION 0x406d1388

/// DOCME
typedef struct tagTHREADNAME_INFO {

	/// DOCME
	DWORD dwType; // must be 0x1000

	/// DOCME
	LPCSTR szName; // pointer to name (in same addr space)

	/// DOCME
	DWORD dwThreadID; // thread ID (-1 caller thread)

	/// DOCME
	DWORD dwFlags; // reserved for future use, most be zero

/// DOCME
} THREADNAME_INFO;

void raiseThreadName(LPCSTR szThreadName) {
	THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = szThreadName;
		info.dwThreadID = GetCurrentThreadId();
		info.dwFlags = 0;
	__try
   {
	  RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
	}
}

void SetThreadName(const char* szThreadName) {
	Poco::Thread* pThread = Poco::Thread::current();
	if(pThread)
		pThread->setName(szThreadName);
	else
		MainThreadName = szThreadName;
	raiseThreadName(szThreadName);
}
 
#else
void SetThreadName(const char* szThreadName) {
	Poco::Thread* pThread = Poco::Thread::current();
	if(pThread)
		pThread->setName(szThreadName);
	else
		MainThreadName = szThreadName;
}
#endif

std::string GetThreadName() {
	return Poco::Thread::current() ? Poco::Thread::current()->name() : MainThreadName;
}