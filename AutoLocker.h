#ifndef _AUTO_LOCK_
#define _AUTO_LOCK_
#pragma once


//#include <Windows.h>
#include "SDL2\include\SDL_mutex.h"


class CLocker
{
public:
	CLocker()
	{
		//::InitializeCriticalSection(&m_access);
		lock = SDL_CreateMutex();
	}
	~CLocker()
	{
		//::DeleteCriticalSection(&m_access);
		SDL_DestroyMutex(lock);
	}
	virtual void Lock()
	{
		//::EnterCriticalSection(&m_access);
		SDL_mutexP(lock);
	}

	virtual void UnLock()
	{
	    //::LeaveCriticalSection(&m_access);
		SDL_mutexV(lock);
	}

private:
	SDL_mutex   *lock;
};

//////////////////////////////////////////////////////////////////////////

class CAutoLocker
{
public:
	CAutoLocker(CLocker* pLocker)
	{
		m_pLocker = pLocker;
		if (m_pLocker)
		{
			m_pLocker->Lock();
		}
	}
	virtual ~CAutoLocker()
	{
		if (m_pLocker)
		{
			m_pLocker->UnLock();
		}
	}



private:
	CLocker* m_pLocker;
};

#endif /*_AUTO_LOCK_*/