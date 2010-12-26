/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_SYNC_POLICY_POCO_H
#define HZ_SYNC_POLICY_POCO_H

#include "hz_config.h"  // feature macros

#include <Poco/Mutex.h>
#include <Poco/RWLock.h>

#include "sync.h"


// Poco-based policy.


namespace hz {


// Native type notes:
// Poco::Mutex is recursive, while Poco::FastMutex is not.
// Poco::ScopedLock doesn't have "bool do_lock", or anything else for that matter.


// Poco doesn't throw any exceptions for these types, nor does this policy.

// We use native types for mutex types here, because they meet the requirements.
// Thus no need to specify locks and other stuff for these separately.

struct SyncPolicyPoco : public SyncScopedLockProvider<SyncPolicyPoco> {

	// Types:

	typedef Poco::FastMutex Mutex;
	typedef Mutex NativeMutex;  // supports lock(), tryLock(), unlock()

	typedef Poco::Mutex RecMutex;
	typedef RecMutex NativeRecMutex;  // supports lock(), tryLock(), unlock()

	typedef Poco::RWLock RWMutex;
	typedef RWMutex NativeRWMutex;  // readLock(), tryReadLock(), writeLock(), tryWriteLock(), unlock().


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
	static bool trylock(Mutex& m) { return m.tryLock(); }
	static void unlock(Mutex& m) { m.unlock(); }

	static void lock(RecMutex& m) { m.lock(); }
	static bool trylock(RecMutex& m) { return m.tryLock(); }
	static void unlock(RecMutex& m) { m.unlock(); }

	static void lock(RWMutex& m, bool for_write = false)
	{
		if (for_write) {
			m.writeLock();
		} else {
			m.readLock();
		}
	}
	static bool trylock(RWMutex& m, bool for_write = false)
	{
		return (for_write ? m.tryWriteLock() : m.tryReadLock());
	}
	static void unlock(RWMutex& m, bool for_write = false)
	{
		m.unlock();
	}

};



// mutex -> policy

template<> struct SyncGetPolicy<SyncPolicyPoco::Mutex> { typedef SyncPolicyPoco type; };
template<> struct SyncGetPolicy<SyncPolicyPoco::RecMutex> { typedef SyncPolicyPoco type; };
template<> struct SyncGetPolicy<SyncPolicyPoco::RWMutex> { typedef SyncPolicyPoco type; };




}  // ns




#endif
