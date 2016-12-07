#include "ThreadFuncs.h"


void* GMXThread::CreateThread(GMXThreadFunc pfunc,void* poParams, unsigned long* pnThreadID)
{
#ifdef _WIN32
  return ::CreateThread(0,0,pfunc,poParams,0,pnThreadID);
#else
  return 0;
#endif
}

bool GMXThread::TerminateThread(void* poThreadHandle, unsigned long nExitCode)
{
#ifdef _WIN32
  return ::TerminateThread(poThreadHandle,nExitCode);
#else
  return true;
#endif
}

bool GMXThread::CloseHandle(void* poHandle)
{
#ifdef _WIN32
  return ::CloseHandle(poHandle);
#else
  return true;
#endif
}


bool GMXThread::GetExitCodeThread(void* poThreadHandle,unsigned long* pnExitCode)
{
#ifdef _WIN32
  return ::GetExitCodeThread(poThreadHandle,pnExitCode);
#else
  return true;
#endif
}

void* GMXThread::CreateSemaphore(long nMinCount,long nMaxCount)
{
#ifdef _WIN32
  return ::CreateSemaphore(0,nMinCount,nMaxCount,0);
#else
  return 0;
#endif
}

bool GMXThread::ReleaseSemaphore(void* poSemaphore,long nReleaseCount)
{
#ifdef _WIN32
  return ::ReleaseSemaphore(poSemaphore,nReleaseCount,0);
#else
  return true;
#endif
}

unsigned long GMXThread::WaitForSingleObject(void* poSemaphore)
{
#ifdef _WIN32
  return ::WaitForSingleObject(poSemaphore,INFINITE);
#else
  return 1;
#endif
}



