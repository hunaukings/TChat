#ifndef _OSCONFIG_
#define _OSCONFIG_
#ifndef WIN32
#include <pthread.h> 
typedef struct sockaddr_in SOCKADDR_IN;

unsigned int GetTickCount()
{
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv,&tz);
	return tv.tv_sec*1000+tv.tv_usec;
}

#define Sleep usleep 
#define ThreadPid pthread_t 
#define ThreadOutStat void*
#define StartThread(process,userptr,threadpid) do \
{\
	pthread_create(threadpid, NULL, process, userptr) ;  \
} while (0); 

#define WaitThread(ThreadPid,Stat) do \
{ \
	pthread_join(ThreadPid, Stat);  \
}while(0)

#else

#include "Windows.h"

#define ThreadPid HANDLE 
#define ThreadOutStat void*
#define StartThread(process,userptr,threadpid) do {\
	HANDLE handle = (HANDLE)_beginthread(process,0,userptr); \
    *threadpid = handle; \
} while (0)
#define WaitThread(ThreadPid,Stat) do { \
	WaitForSingleObject(ThreadPid,INFINITE); \
}while(0) 
#endif



#endif /*_OSCONFIG_*/