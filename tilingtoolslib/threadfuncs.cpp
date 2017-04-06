#include "threadfuncs.h"

/*
#ifdef _WIN32
#define GMXTHREADWIN
#endif



bool GMXThread::CloseHandle(void* poHandle)
{
#ifdef GMXTHREADWIN
  return ::CloseHandle(poHandle);
#else
  return true;
#endif
}


void* GMXThread::CreateSemaphore(long nMinCount,long nMaxCount)
{
#ifdef GMXTHREADWIN
  return ::CreateSemaphore(0,nMinCount,nMaxCount,0);
#else
  return 0;
#endif
}

bool GMXThread::ReleaseSemaphore(void* poSemaphore,long nReleaseCount)
{
#ifdef GMXTHREADWIN
  return ::ReleaseSemaphore(poSemaphore,nReleaseCount,0);
#else
  return true;
#endif
}

unsigned long GMXThread::WaitForSingleObject(void* poSemaphore)
{
#ifdef GMXTHREADWIN
  return ::WaitForSingleObject(poSemaphore,INFINITE);
#else
  return 1;
#endif
}
*/


