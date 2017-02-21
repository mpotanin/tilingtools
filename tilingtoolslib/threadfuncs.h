#pragma once
#include "stdafx.h"

#ifdef _WIN32
typedef unsigned long (WINAPI *GMXThreadFunc)(void*); 
#else

#endif

class GMXThread
{
public:

  static void* CreateThread(GMXThreadFunc pfunc,void* poParams, unsigned long* pnThreadID);
  static bool TerminateThread(void* poThreadHandle, unsigned long nExitCode);
  static bool CloseHandle(void* poHandle);
  static bool GetExitCodeThread(void* poThreadHandle,unsigned long* pnExitCode);
  static void* CreateSemaphore(long nMinCoutn,long nMaxCount);
  static bool ReleaseSemaphore(void* poSemaphore,long nReleaseCount);
  static unsigned long WaitForSingleObject(void* poSemaphore); 
  
};