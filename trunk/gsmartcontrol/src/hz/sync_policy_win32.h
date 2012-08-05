/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_SYNC_POLICY_WIN32_H
#define HZ_SYNC_POLICY_WIN32_H

#include "hz_config.h"  // feature macros

#ifndef _WIN32
	#error Cannot compile a win32-only file under non-win32 system
#endif

#include <windows.h>

// Don't use DBG_ASSERT() here, it's using this library and therefore
// too error-prone to use in this context.
#include <cassert>
#ifndef ASSERT  // assert() is undefined if NDEBUG is defined.
	#define ASSERT(a) assert(a)
#endif

#include "sync.h"


/**
\file
Win32-Threads-based policy.
*/


namespace hz {



/// C++ wrapper for Win32 CRITICAL_SECTION.
/// Note: Win32 CRITICAL_SECTION is always recursive.
template<int TypeChanger>
class MutexWin32 : public hz::noncopyable {
	public:
		typedef CRITICAL_SECTION native_type;

		static void native_lock(native_type& mutex)
		{
			try {
				EnterCriticalSection(&mutex);
			}
			catch (...) {
				THROW_FATAL(sync_resource_error("MutexWin32::native_lock(): Error locking mutex."));
			}
		}

		static bool native_trylock(native_type& mutex)
		{
			bool res = 0;
			try {
				res = TryEnterCriticalSection(&mutex);
			}
			catch (...) {
				THROW_FATAL(sync_resource_error("MutexWin32::native_trylock(): Error trying to lock mutex."));
			}
			return res;
		}

		static void native_unlock(native_type& mutex)
		{
			try {
				LeaveCriticalSection(&mutex);
			}
			catch (...) {
				THROW_FATAL(sync_resource_error("MutexWin32::native_unlock(): Error unlocking mutex."));
			}
		}


		MutexWin32()
		{
			try {
				// MSDN says 4000 is a good value. has effect only for SMP.
				if (!InitializeCriticalSectionAndSpinCount(&cs_, 4000))
					THROW_FATAL(sync_resource_error("MutexWin32::MutexWin32(): Error creating mutex."));
			}
			catch (...) {
				THROW_FATAL(sync_resource_error("MutexWin32::MutexWin32(): Error creating mutex."));
			}
		}

		~MutexWin32()
		{
			try {
				DeleteCriticalSection(&cs_);
			}
			catch (...) {
				THROW_FATAL(sync_resource_error("MutexWin32::MutexWin32(): Error destroying mutex."));
			}
		}

		void lock() { native_lock(cs_); }
		bool trylock() { return native_trylock(cs_); }
		void unlock() { native_unlock(cs_); }

	private:
		CRITICAL_SECTION cs_;

};




/// Implementation of RW lock for Win32.
class RWMutexWin32 : public hz::noncopyable {

	public:

		RWMutexWin32() : readers_(0), writers_(0)
		{
			if ((mutex_ = CreateMutexW(NULL, false, NULL)) != NULL) {
				if ((read_event_ = CreateEventW(NULL, true, true, NULL)) != NULL) {
					if ((write_event_ = CreateEventW(NULL, true, true, NULL)) != NULL) {
						return;
					}
				}
			}
			THROW_FATAL(sync_resource_error("RWMutexWin32::RWMutexWin32(): Error creating read/write lock."));
		}


		~RWMutexWin32()
		{
			try {
				if (mutex_)
					CloseHandle(mutex_);
				if (read_event_)
					CloseHandle(read_event_);
				if (write_event_)
					CloseHandle(write_event_);
			}
			catch (...) {
				THROW_FATAL(sync_resource_error("RWMutexWin32::~RWMutexWin32(): Error destroying read/write lock."));
			}
		}


		void lock(bool for_write = false)
		{
			if (for_write) {
				add_writer();

				HANDLE h[2] = {mutex_, write_event_};
				DWORD res = WaitForMultipleObjects(2, h, TRUE, INFINITE);
				if (res == WAIT_OBJECT_0 || res == (WAIT_OBJECT_0 + 1)) {
					--writers_;
					++readers_;
					ResetEvent(read_event_);
					ResetEvent(write_event_);
					ReleaseMutex(mutex_);

				} else {
					remove_writer();
					THROW_FATAL(sync_resource_error("RWMutexWin32::lock(): Error write-locking a read/write lock."));
				}

			} else {  // read:
				HANDLE h[2] = {mutex_, read_event_};
				DWORD res = WaitForMultipleObjects(2, h, TRUE, INFINITE);
				if (res == WAIT_OBJECT_0 || res == (WAIT_OBJECT_0 + 1)) {
					++readers_;
					ResetEvent(write_event_);
					ReleaseMutex(mutex_);

				} else {
					THROW_FATAL(sync_resource_error("RWMutexWin32::lock(): Error read-locking a read/write lock."));
				}
			}
		}


		bool trylock(bool for_write = false)
		{
			if (for_write) {
				add_writer();

				HANDLE h[2] = {mutex_, write_event_};
				DWORD res = WaitForMultipleObjects(2, h, TRUE, 1);
				if (res == WAIT_OBJECT_0 || res == (WAIT_OBJECT_0 + 1)) {
					--writers_;
					++readers_;
					ResetEvent(read_event_);
					ResetEvent(write_event_);
					ReleaseMutex(mutex_);
					return true;

				} else {
					remove_writer();
					if (res != WAIT_TIMEOUT)
						THROW_FATAL(sync_resource_error("RWMutexWin32::trylock(): Error while trying to write-lock a read/write lock."));
					return false;
				}

			} else {  // read:
				HANDLE h[2] = {mutex_, read_event_};
				DWORD res = WaitForMultipleObjects(2, h, TRUE, 1);
				if (res == WAIT_OBJECT_0 || res == (WAIT_OBJECT_0 + 1)) {
					++readers_;
					ResetEvent(write_event_);
					ReleaseMutex(mutex_);
					return true;

				} else {
					if (res != WAIT_TIMEOUT)
						THROW_FATAL(sync_resource_error("RWMutexWin32::trylock(): Error while trying to read-lock a read/write lock."));
					return false;
				}
			}
		}


		void unlock(bool for_write = false)
		{
			if (WaitForSingleObject(mutex_, INFINITE) == WAIT_OBJECT_0) {
				if (writers_ == 0)
					SetEvent(read_event_);
				if (--readers_ == 0)
					SetEvent(write_event_);
				ReleaseMutex(mutex_);
				return;
			}
			THROW_FATAL(sync_resource_error("RWMutexWin32::unlock(): Error while unlocking a read/write lock."));
		}


	private:

		void add_writer()
		{
			if (WaitForSingleObject(mutex_, INFINITE) == WAIT_OBJECT_0) {
				if (++writers_ == 1)
					ResetEvent(read_event_);
				ReleaseMutex(mutex_);
				return;
			}
			THROW_FATAL(sync_resource_error("RWMutexWin32::add_writer(): Error while locking a read/write lock."));
		}


		void remove_writer()
		{
			if (WaitForSingleObject(mutex_, INFINITE) == WAIT_OBJECT_0) {
				if (--writers_ == 0)
					SetEvent(read_event_);
				ReleaseMutex(mutex_);
				return;
			}
			THROW_FATAL(sync_resource_error("RWMutexWin32::remove_writer(): Error while locking a read/write lock."));
		}


		HANDLE mutex_;
		HANDLE read_event_;
		HANDLE write_event_;
		unsigned int readers_;
		unsigned int writers_;

};





/// Win32-threads policy.
/// Native type notes: none.
/// Note: We don't know if we trap all the error conditions - one can never be sure with win32.
struct SyncPolicyWin32 : public SyncScopedLockProvider<SyncPolicyWin32> {

	// Types:

	typedef MutexWin32<1> Mutex;
	typedef Mutex::native_type NativeMutex;

	typedef MutexWin32<2> RecMutex;
	typedef RecMutex::native_type NativeRecMutex;  ///< same type as NativeMutex

	typedef RWMutexWin32 RWMutex;
	typedef RWMutex NativeRWMutex;  ///< win32 doesn't have a native RWMutex-like type, so use the same type.


	typedef GenericScopedLock<Mutex> ScopedLock;
	typedef GenericScopedTryLock<Mutex> ScopedTryLock;

	typedef GenericScopedLock<NativeMutex> ScopedNativeLock;
	typedef GenericScopedTryLock<NativeMutex> ScopedNativeTryLock;

	typedef GenericScopedLock<RecMutex> ScopedRecLock;
	typedef GenericScopedTryLock<RecMutex> ScopedRecTryLock;

	typedef GenericScopedLock<NativeRecMutex> ScopedNativeRecLock;
	typedef GenericScopedTryLock<NativeRecMutex> ScopedNativeRecTryLock;

	typedef GenericScopedRWLock<RWMutex> ScopedRWLock;
	typedef GenericScopedRWTryLock<RWMutex> ScopedRWTryLock;

	typedef GenericScopedRWLock<NativeRWMutex> ScopedNativeRWLock;
	typedef GenericScopedRWTryLock<NativeRWMutex> ScopedNativeRWTryLock;


	// Static methods

	static bool init() { return true; }

	static void lock(Mutex& m) { m.lock(); }
	static bool trylock(Mutex& m) { return m.trylock(); }
	static void unlock(Mutex& m) { m.unlock(); }

	static void lock(NativeMutex& m) { Mutex::native_lock(m); }
	static bool trylock(NativeMutex& m) { return Mutex::native_trylock(m); }
	static void unlock(NativeMutex& m) { Mutex::native_unlock(m); }

	static void lock(RecMutex& m) { m.lock(); }
	static bool trylock(RecMutex& m) { return m.trylock(); }
	static void unlock(RecMutex& m) { m.unlock(); }

// 	static void lock(NativeRecMutex& m) { RecMutex::native_lock(m); }
// 	static bool trylock(NativeRecMutex& m) { return RecMutex::native_trylock(m); }
// 	static void unlock(NativeRecMutex& m) { RecMutex::native_unlock(m); }

	static void lock(RWMutex& m, bool for_write = false) { m.lock(for_write); }
	static bool trylock(RWMutex& m, bool for_write = false) { return m.trylock(for_write); }
	static void unlock(RWMutex& m, bool for_write = false) { m.unlock(for_write); }

// 	static void lock(NativeRWMutex& m, bool for_write = false) { RWMutex::native_lock(m, for_write); }
// 	static bool trylock(NativeRWMutex& m, bool for_write = false) { return RWMutex::native_trylock(m, for_write); }
// 	static void unlock(NativeRWMutex& m, bool for_write = false) { RWMutex::native_unlock(m, for_write); }

};



// mutex -> policy

template<> struct SyncGetPolicy<SyncPolicyWin32::Mutex> { typedef SyncPolicyWin32 type; };
template<> struct SyncGetPolicy<SyncPolicyWin32::NativeMutex> { typedef SyncPolicyWin32 type; };
template<> struct SyncGetPolicy<SyncPolicyWin32::RecMutex> { typedef SyncPolicyWin32 type; };
// template<> struct SyncGetPolicy<SyncPolicyWin32::NativeRecMutex> { typedef SyncPolicyWin32 type; };
template<> struct SyncGetPolicy<SyncPolicyWin32::RWMutex> { typedef SyncPolicyWin32 type; };
// template<> struct SyncGetPolicy<SyncPolicyWin32::NativeRWMutex> { typedef SyncPolicyWin32 type; };





}  // ns




#endif
