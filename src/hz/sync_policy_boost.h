/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_SYNC_POLICY_BOOST_H
#define HZ_SYNC_POLICY_BOOST_H

#include "hz_config.h"  // feature macros

#include <string>

#include "sync.h"


// Boost.Thread-based policy.


// ----------------------------- Boost 1.35 version


#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/shared_mutex.hpp>



namespace hz {




// Native type notes:
// Boost scoped_lock doesn't provide the do_lock parameter anymore.
// It has been replaced by boost::defer_lock_type argument.
// There is no single lock for shared_mutex - it has shared_lock and unique_lock.

// We use native types for mutex types here, because they meet the requirements.
// Thus no need to specify locks and other stuff for these separately.

struct SyncPolicyBoost : public SyncScopedLockProvider<SyncPolicyBoost> {

	// Types:

	typedef boost::mutex Mutex;
	typedef Mutex NativeMutex;  // supports lock(), try_lock(), unlock()

	typedef boost::recursive_mutex RecMutex;
	typedef RecMutex NativeRecMutex;  // supports lock(), try_lock(), unlock()

	typedef boost::shared_mutex RWMutex;
	typedef RWMutex NativeRWMutex;  // lock(), try_lock(), unlock() - exclusive. *_shared() - shared.


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
		return true;
	}


	static void lock(Mutex& m)
	{
		try {
			m.lock();
		}
		catch (boost::thread_resource_error& ex) {
			throw_boost_to_sync_exception(ex, "SyncPolicyBoost::lock(Mutex): Error while locking mutex.");
		}
	}

	static bool trylock(Mutex& m)
	{
		bool status = false;
		try {
			status = m.try_lock();
		}
		catch (boost::thread_resource_error& ex) {
			throw_boost_to_sync_exception(ex, "SyncPolicyBoost::trylock(Mutex): Error while trying to lock mutex.");
		}
		return status;
	}

	static void unlock(Mutex& m)
	{
		m.unlock();  // this doesn't throw anything
	}


	static void lock(RecMutex& m)
	{
		try {
			m.lock();
		}
		catch (boost::thread_resource_error& ex) {
			throw_boost_to_sync_exception(ex, "SyncPolicyBoost::lock(RecMutex): Error while locking mutex.");
		}
	}

	static bool trylock(RecMutex& m)
	{
		bool status = false;
		try {
			status = m.try_lock();
		}
		catch (boost::thread_resource_error& ex) {
			throw_boost_to_sync_exception(ex, "SyncPolicyBoost::trylock(RecMutex): Error while trying to lock mutex.");
		}
		return status;
	}

	static void unlock(RecMutex& m)
	{
		m.unlock();  // this doesn't throw anything
	}


	static void lock(RWMutex& m, bool for_write = false)
	{
		try {
			if (for_write) {
				m.lock();
			} else {
				m.lock_shared();
			}
		}
		catch (boost::thread_resource_error& ex) {
			throw_boost_to_sync_exception(ex, std::string("SyncPolicyBoost::trylock(RWMutex, ")
					+ (for_write ? "true" : "false") + "): Error while locking mutex.");
		}
	}

	static bool trylock(RWMutex& m, bool for_write = false)
	{
		bool status = false;
		try {
			status = (for_write ? m.try_lock() : m.try_lock_shared());
		}
		catch (boost::thread_resource_error& ex) {
			throw_boost_to_sync_exception(ex, std::string("SyncPolicyBoost::trylock(RWMutex, ")
					+ (for_write ? "true" : "false") + "): Error while trying to lock mutex.");
		}
		return status;
	}

	static void unlock(RWMutex& m, bool for_write = false)
	{
		if (for_write) {
			m.unlock();  // doesn't throw
		} else {
			m.unlock_shared();  // doesn't throw
		}
	}


	private:

		static void throw_boost_to_sync_exception(const boost::thread_resource_error& ex, const std::string& smsg)
		{
			const char* msg = ex.what();
			if (msg) {
				THROW_FATAL(sync_resource_error(smsg
						+ " Original boost::thread_resource_error exception message: \"" + msg + "\"."));
			} else {
				THROW_FATAL(sync_resource_error(smsg
						+ " This exception was thrown after boost::thread_resource_error was caught (it didn't contain any message)."));
			}
		}

};



// mutex -> policy

template<> struct SyncGetPolicy<SyncPolicyBoost::Mutex> { typedef SyncPolicyBoost type; };
template<> struct SyncGetPolicy<SyncPolicyBoost::RecMutex> { typedef SyncPolicyBoost type; };
template<> struct SyncGetPolicy<SyncPolicyBoost::RWMutex> { typedef SyncPolicyBoost type; };




}  // ns





// ----------------------------- Boost 1.34 version (outdated!)

/*
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
// #include <boost/thread/read_write_mutex.hpp>


namespace hz {




// Provide custom ScopedRWLock class.
// class ScopedRWLockBoost : public boost::read_write_mutex::scoped_read_write_lock {
// 	public:
// 		ScopedRWLockBoost(boost::read_write_mutex& mutex, bool for_write = false, bool do_lock = true)
// 				: boost::read_write_mutex::scoped_read_write_lock(mutex, do_lock ?
// 					(for_write ? boost::read_write_lock_state::write_locked : boost::read_write_lock_state::read_locked)
// 					: boost::read_write_lock_state::unlocked)
// 		{ }
// };



// Native type notes:
// Boost mutex types support locking through [mutex_type]::scoped_*lock objects only.
// We use somewhat undocumented static functions of boost::detail::thread::*lock_ops.

// Note: Boost Read/Write Mutex is documented, but is unavailable in source form
// as of boost 1.34.1. That is, there were some implementation problems in boost, so
// it was removed. The policy uses the documented API, but it's disabled until boost
// provides a working implementation.


// Classes may use this in single-threaded or non-locking environments
struct SyncPolicyBoost {

	typedef boost::try_mutex Mutex;  // doesn't have any usable methods
	typedef boost::try_mutex::scoped_lock ScopedLock;  // works well

	typedef boost::recursive_try_mutex RecMutex;  // doesn't have any usable methods
	typedef boost::recursive_try_mutex::scoped_lock ScopedRecLock;  // works well

// 	typedef boost::try_read_write_mutex RWMutex;  // doesn't have any usable methods
// 	typedef ScopedRWLockBoost ScopedRWLock;  // needs a wrapper

	static void lock(Mutex& m) { boost::detail::thread::lock_ops<Mutex>::lock(m); }
	static bool trylock(Mutex& m) { return boost::detail::thread::lock_ops<Mutex>::trylock(m); }
	static void unlock(Mutex& m) { boost::detail::thread::lock_ops<Mutex>::unlock(m); }

	static void lock(RecMutex& m) { boost::detail::thread::lock_ops<RecMutex>::lock(m); }
	static bool trylock(RecMutex& m) { return boost::detail::thread::lock_ops<RecMutex>::trylock(m); }
	static void unlock(RecMutex& m) { boost::detail::thread::lock_ops<RecMutex>::unlock(m); }

// 	static void lock(RWMutex& m, bool for_write = false)
// 	{
// 		if (for_write) {
// 			boost::detail::thread::read_write_lock_ops<RWMutex>::write_lock(m);
// 		} else {
// 			boost::detail::thread::read_write_lock_ops<RWMutex>::read_lock(m);
// 		}
// 	}
// 	static bool trylock(RWMutex& m, bool for_write = false)
// 	{
// 		if (for_write)
// 			return boost::detail::thread::read_write_lock_ops<RWMutex>::try_write_lock(m);
// 	}
// 	static void unlock(RWMutex& m, bool for_write = false)
// 	{
// 		if (for_write) {
// 			boost::detail::thread::read_write_lock_ops<RWMutex>::write_unlock(m);
// 		} else {
// 			boost::detail::thread::read_write_lock_ops<RWMutex>::read_unlock(m);
// 		}
// 	}

};



// mutex -> policy

template<> struct SyncGetPolicy<SyncPolicyBoost::Mutex> {
	typedef SyncPolicyBoost type;
};

template<> struct SyncGetPolicy<SyncPolicyBoost::RecMutex> {
	typedef SyncPolicyBoost type;
};

// template<> struct SyncGetPolicy<SyncPolicyNone::RWMutex> {
// 	typedef SyncPolicyNone type;
// };


}  // ns
*/




#endif
