#ifndef __IMSLIB_LOCK_HPP_UX
#define __IMSLIB_LOCK_HPP_UX

#if (_MSC_VER >= 800)
#pragma once
#endif

#ifndef __cplusplus
#error You must use c++ compiler.
#endif//__cplusplus

#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#define INFINITE -1
typedef pthread_mutex_t CRITICAL_SECTION;
inline int InitializeCriticalSection(CRITICAL_SECTION * cs)
{
	return pthread_mutex_init(cs, NULL);
}
#define DeleteCriticalSection pthread_mutex_destroy
#define EnterCriticalSection pthread_mutex_lock
#define LeaveCriticalSection pthread_mutex_unlock
#endif//_WIN32

namespace imsux {

struct CriticalSectionLocker {
	CriticalSectionLocker(CRITICAL_SECTION & cs) : mcs(cs) {}
	inline void Lock(unsigned) {
		EnterCriticalSection(&mcs);
	}
	inline void UnLock() {
		LeaveCriticalSection(&mcs);
	}
	CRITICAL_SECTION & mcs;
};

#ifdef _WIN32
struct MutexLocker {
	MutexLocker(HANDLE mutex) : mMutex(mutex) {}
	inline void Lock(unsigned time) {
		WaitForSingleObject(mMutex, time);
	}
	inline void UnLock() {
		ReleaseMutex(mMutex);
	}
	HANDLE mMutex;
};
struct SemaphoreLocker {
	SemaphoreLocker(HANDLE sem) : mSem(sem) {}
	inline void Lock(unsigned time) {
		WaitForSingleObject(mSem, time);
	}
	inline void UnLock() {
		ReleaseSemaphore(mSem, 1, NULL);
	}
	HANDLE mSem;
};
#endif//_WIN32

template <class T>
struct Locker {
	Locker(T & t, unsigned dt) : i(0), mt(t) {
		mt.Lock(dt);
	}
	~Locker() {
		mt.UnLock();
	}
	int i; T & mt;
};

} // namespace imsux

#define _ims_leave_lock() break
#define _ims_lock(T, p) \
	if (0); else for (imsux::Locker < T > l( p, (unsigned)INFINITE ); l.i < 1; l.i++)
#define _ims_lockex(T, p, t) \
	if (0); else for (imsux::Locker < T > l( p, t ); l.i < 1; l.i++)

#endif//__IMSLIB_LOCK_HPP_UX
