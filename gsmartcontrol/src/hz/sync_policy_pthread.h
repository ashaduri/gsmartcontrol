/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_SYNC_POLICY_PTHREAD_H
#define HZ_SYNC_POLICY_PTHREAD_H

#include "hz_config.h"  // feature macros

#include <pthread.h>
#include <cerrno>  // EBUSY
#include <string>

// Don't use DBG_ASSERT() here, it's using this library and therefore
// too error-prone to use in this context.
#include <cassert>
#ifndef ASSERT  // assert() is undefined if NDEBUG is defined.
	#define ASSERT(a) assert(a)
#endif

#include "errno_string.h"  // errno_string
#include "sync.h"


// Posix Threads-based policy.

// Note: RecMutex assumes that pthread_mutexattr_settype
// and PTHREAD_MUTEX_RECURSIVE work on current platform.

// NOTE: This file requires UNIX98 support, enabled via _XOPEN_SOURCE >= 500
// in glibc. Solaris doesn't need any additional feature macros.

// Configuration macros:
// This enables PTHREAD_MUTEX_ERRORCHECK flag on
// non-recursive mutexes, which may perform some additional
// checking on some systems.
#ifndef HZ_SYNC_PTHREAD_ERROR_CHECKS
	#define HZ_SYNC_PTHREAD_ERROR_CHECKS 1
#endif



namespace hz {


namespace internal {

	inline void sync_pthread_throw_exception(const std::string& msg, int errno_value = 0)
	{
		if (errno_value == 0) {
			THROW_FATAL(sync_resource_error(msg));
		} else {
			THROW_FATAL(sync_resource_error(msg + " Errno: " + hz::errno_string(errno_value)));
		}
	}

}



// Attempting to destroy a locked mutex results in undefined behavior.
class MutexPthread : public hz::noncopyable {
	public:

		typedef pthread_mutex_t native_type;

		static void native_lock(native_type& mutex)
		{
			int res = pthread_mutex_lock(&mutex);
			if (res != 0) {
				internal::sync_pthread_throw_exception("MutexPthread::native_lock(): Error locking mutex.", res);
			}
		}

		static bool native_trylock(native_type& mutex)
		{
			int res = pthread_mutex_trylock(&mutex);
			if (res == EBUSY) {  // the only valid non-zero result
				return false;
			}
			if (res != 0) {
				internal::sync_pthread_throw_exception("MutexPthread::native_trylock(): Error while trying to lock mutex.", res);
			}
			return true;
		}

		static void native_unlock(native_type& mutex)
		{
			int res = pthread_mutex_unlock(&mutex);
			if (res != 0) {
				internal::sync_pthread_throw_exception("MutexPthread::native_unlock(): Error unlocking mutex.", res);
			}
		}


		MutexPthread()
		{
#if defined HZ_SYNC_PTHREAD_ERROR_CHECKS && HZ_SYNC_PTHREAD_ERROR_CHECKS
			pthread_mutexattr_t attr;
			int res = pthread_mutexattr_init(&attr);
			if (res != 0) {
				internal::sync_pthread_throw_exception("MutexPthread::MutexPthread(): Error creating mutex attributes.", res);
			}

			res = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
			if (res != 0) {
				internal::sync_pthread_throw_exception("MutexPthread::MutexPthread(): Error setting mutex attributes.", res);
				pthread_mutexattr_destroy(&attr);  // cleanup
			}

			res = pthread_mutex_init(&mutex_, &attr);
			if (res != 0) {
				internal::sync_pthread_throw_exception("MutexPthread::MutexPthread(): Error initializing mutex.", res);
				pthread_mutexattr_destroy(&attr);  // cleanup
			}

			res = pthread_mutexattr_destroy(&attr);
			if (res != 0) {
				internal::sync_pthread_throw_exception("MutexPthread::MutexPthread(): Error destroying mutex attributes.", res);
				pthread_mutex_destroy(&mutex_);  // avoid undetermined state
			}

#else
			int res = pthread_mutex_init(&mutex_, NULL);
			if (res != 0) {
				internal::sync_pthread_throw_exception("MutexPthread::MutexPthread(): Error initializing mutex.", res);
			}

#endif
		}

		~MutexPthread()
		{
			int res = pthread_mutex_destroy(&mutex_);
			if (res != 0) {
				internal::sync_pthread_throw_exception("MutexPthread::~MutexPthread(): Error destroying mutex.", res);
			}
		}

		void lock() { native_lock(mutex_); }
		bool trylock() { return native_trylock(mutex_); }
		void unlock() { native_unlock(mutex_); }

	private:
		pthread_mutex_t mutex_;

};




class RecMutexPthread : public hz::noncopyable {
	public:

		typedef pthread_mutex_t native_type;

		static void native_lock(native_type& mutex)
		{
			int res = pthread_mutex_lock(&mutex);
			if (res != 0) {
				internal::sync_pthread_throw_exception("RecMutexPthread::native_lock(): Error locking mutex.", res);
			}
		}

		static bool native_trylock(native_type& mutex)
		{
			int res = pthread_mutex_trylock(&mutex);
			if (res == EBUSY) {  // the only valid non-zero result
				return false;
			}
			if (res != 0) {
				internal::sync_pthread_throw_exception("RecMutexPthread::native_trylock(): Error while trying to lock mutex.", res);
			}
			return true;
		}

		static void native_unlock(native_type& mutex)
		{
			int res = pthread_mutex_unlock(&mutex);
			if (res != 0) {
				internal::sync_pthread_throw_exception("RecMutexPthread::native_unlock(): Error unlocking mutex.", res);
			}
		}


		RecMutexPthread() : count_(0)
		{
			pthread_mutexattr_t attr;
			int res = pthread_mutexattr_init(&attr);
			if (res != 0) {
				internal::sync_pthread_throw_exception("RecMutexPthread::RecMutexPthread(): Error creating mutex attributes.", res);
			}

			// may need -D__USE_UNIX98 on non-gcc (sun, pgi)
			res = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
			if (res != 0) {
				internal::sync_pthread_throw_exception("RecMutexPthread::RecMutexPthread(): Error setting mutex attributes.", res);
				pthread_mutexattr_destroy(&attr);  // cleanup
			}

			res = pthread_mutex_init(&mutex_, &attr);
			if (res != 0) {
				internal::sync_pthread_throw_exception("RecMutexPthread::RecMutexPthread(): Error initializing mutex.", res);
				pthread_mutexattr_destroy(&attr);  // cleanup
			}

			res = pthread_mutexattr_destroy(&attr);
			if (res != 0) {
				internal::sync_pthread_throw_exception("RecMutexPthread::RecMutexPthread(): Error destroying mutex attributes.", res);
				pthread_mutex_destroy(&mutex_);  // avoid undetermined state
			}
		}

		~RecMutexPthread()
		{
			int res = pthread_mutex_destroy(&mutex_);
			if (res != 0) {
				internal::sync_pthread_throw_exception("RecMutexPthread::~RecMutexPthread(): Error destroying mutex.", res);
			}
		}


		// Note: While pthread_mutex_t is recursively-lockable by its own,
		// these functions provide the benefit of avoiding a lock depth of
		// more than two, thus working around some system limits.
		// There's an additional checking as a bonus.

		void lock()
		{
			native_lock(mutex_);
			if (++count_ > 1)
				native_unlock(mutex_);
		}

		bool trylock()
		{
			if (native_trylock(mutex_) == false)
				return false;
			if (++count_ > 1)
				native_unlock(mutex_);
			return true;
		}

		void unlock()
		{
			if (count_ <= 0)
				internal::sync_pthread_throw_exception("RecMutexPthread::unlock(): Count underflow while trying to unlock a mutex.");
			if (--count_ == 0)
				native_unlock(mutex_);
		}

	private:
		pthread_mutex_t mutex_;
		unsigned count_;

};




class RWMutexPthread : public hz::noncopyable {
	public:

		typedef pthread_rwlock_t native_type;

		static void native_lock(native_type& mutex, bool for_write)
		{
			if (for_write) {
				int res = pthread_rwlock_wrlock(&mutex);
				if (res != 0) {
					internal::sync_pthread_throw_exception("RWMutexPthread::native_unlock(): Error write-locking a read/write lock.", res);
				}
			} else {
				int res = pthread_rwlock_rdlock(&mutex);
				if (res != 0) {
					internal::sync_pthread_throw_exception("RWMutexPthread::native_unlock(): Error read-locking a read/write lock.", res);
				}
			}
		}

		static bool native_trylock(native_type& mutex, bool for_write)
		{
			if (for_write) {
				int res = pthread_rwlock_trywrlock(&mutex);
				if (res == 0)
					return true;
				if (res != EBUSY)
					internal::sync_pthread_throw_exception("RWMutexPthread::native_trylock(): Error trying to write-lock a read/write lock.", res);

			} else {  // read
				int res = pthread_rwlock_tryrdlock(&mutex);
				if (res == 0)
					return true;
				if (res != EBUSY)
					internal::sync_pthread_throw_exception("RWMutexPthread::native_trylock(): Error trying to read-lock a read/write lock.", res);
			}

			return false;
		}

		static void native_unlock(native_type& mutex, bool for_write)
		{
			int res = pthread_rwlock_unlock(&mutex);
			if (res != 0)
				internal::sync_pthread_throw_exception("RWMutexPthread::native_unlock(): Error while unlocking a read/write lock.", res);
		}


		RWMutexPthread()
		{
			int res = pthread_rwlock_init(&rwl_, NULL);
			if (res != 0)
				internal::sync_pthread_throw_exception("RWMutexPthread::RWMutexPthread(): Error while creating a read/write lock.", res);
		}

		~RWMutexPthread()
		{
			int res = pthread_rwlock_destroy(&rwl_);
			if (res != 0)
				internal::sync_pthread_throw_exception("RWMutexPthread::RWMutexPthread(): Error while destroying a read/write lock.", res);
		}

		void lock(bool for_write = false) { native_lock(rwl_, for_write); }
		bool trylock(bool for_write = false) { return native_trylock(rwl_, for_write); }
		void unlock(bool for_write = false) { native_unlock(rwl_, for_write); }

	private:
		pthread_rwlock_t rwl_;

};





// Native type notes: none.


struct SyncPolicyPthread : public SyncScopedLockProvider<SyncPolicyPthread> {

	// Types:

	typedef MutexPthread Mutex;
	typedef Mutex::native_type NativeMutex;

	typedef RecMutexPthread RecMutex;
	typedef RecMutex::native_type NativeRecMutex;  // same type as NativeMutex

	typedef RWMutexPthread RWMutex;
	typedef RWMutex::native_type NativeRWMutex;


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

	static bool init()
	{
	#ifdef PTW32_STATIC_LIB
		ptw32_processInitialize();  // needed only when linking statically to pthreads-win32
	#endif
		return true;
	}

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

	static void lock(NativeRWMutex& m, bool for_write = false) { RWMutex::native_lock(m, for_write); }
	static bool trylock(NativeRWMutex& m, bool for_write = false) { return RWMutex::native_trylock(m, for_write); }
	static void unlock(NativeRWMutex& m, bool for_write = false) { RWMutex::native_unlock(m, for_write); }

};



// mutex -> policy

template<> struct SyncGetPolicy<SyncPolicyPthread::Mutex> { typedef SyncPolicyPthread type; };
template<> struct SyncGetPolicy<SyncPolicyPthread::NativeMutex> { typedef SyncPolicyPthread type; };
template<> struct SyncGetPolicy<SyncPolicyPthread::RecMutex> { typedef SyncPolicyPthread type; };
// template<> struct SyncGetPolicy<SyncPolicyPthread::NativeRecMutex> { typedef SyncPolicyPthread type; };
template<> struct SyncGetPolicy<SyncPolicyPthread::RWMutex> { typedef SyncPolicyPthread type; };
template<> struct SyncGetPolicy<SyncPolicyPthread::NativeRWMutex> { typedef SyncPolicyPthread type; };





}  // ns




#endif
