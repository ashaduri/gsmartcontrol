/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_SYNC_POLICY_GLIBMM_H
#define HZ_SYNC_POLICY_GLIBMM_H

#include "hz_config.h"  // feature macros

#include <glibmm/thread.h>

#include "sync.h"


/**
\file
Glibmm-based policy.
*/


namespace hz {


/// Note: For this policy to work, you MUST initialize GThread system
/// by calling one of the following (doesn't matter which exactly) beforehand:
/// Glib::thread_init();  // glibmm
/// or
/// g_thread_init(NULL);  // glib
/// or
/// SyncPolicyGlibmm::init();  // sync's wrapper
///
/// Note: Unlike Glib::RecMutex (and the types in Glib policy), Glib::Mutex will give
/// errors if GThread is not initialized (as described above). Glib::RecMutex and
/// Glib::RWLock will just silently do nothing.
///
/// Native type notes:
/// Glib::Mutex::Lock has acquire() / release().
/// Glib::RecMutex::Lock has acquire() / release(). Different class than Glib::Mutex::Lock.
/// Glib::RWLock has reader_lock() / writer_lock() / reader_unlock() / writer_unlock().
/// Glib::RWLock::ReaderLock and Glib::RWLock::WriterLock are _two_ classes.
///
/// Glibmm doesn't throw any exceptions for these types, nor does this policy.
///
/// We use native types for mutex types here, because they meet the requirements.
/// Thus no need to specify locks and other stuff for these separately.
struct SyncPolicyGlibmm : public SyncScopedLockProvider<SyncPolicyGlibmm> {

	// Types:

	typedef Glib::Mutex Mutex;
	typedef Mutex NativeMutex;  ///< supports lock(), unlock()

	typedef Glib::RecMutex RecMutex;
	typedef RecMutex NativeRecMutex;  ///< supports lock(), unlock()

	typedef Glib::RWLock RWMutex;
	typedef RWMutex NativeRWMutex;  ///< reader_(|try|un)lock(), writer_(|try|un)lock()


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

	/// If glibmm threads are unavailable, this will abort.
	static bool init() { if (!Glib::thread_supported()) Glib::thread_init(); return true; }

	static void lock(Mutex& m) { m.lock(); }
	static bool trylock(Mutex& m) { return m.trylock(); }
	static void unlock(Mutex& m) { m.unlock(); }

	static void lock(RecMutex& m) { m.lock(); }
	static bool trylock(RecMutex& m) { return m.trylock(); }
	static void unlock(RecMutex& m) { m.unlock(); }

	static void lock(RWMutex& m, bool for_write = false)
	{
		if (for_write) {
			m.writer_lock();
		} else {
			m.reader_lock();
		}
	}
	static bool trylock(RWMutex& m, bool for_write = false)
	{
		return (for_write ? m.writer_trylock() : m.reader_trylock());
	}
	static void unlock(RWMutex& m, bool for_write = false)
	{
		if (for_write) {
			m.writer_unlock();
		} else {
			m.reader_unlock();
		}
	}

};



// mutex -> policy

template<> struct SyncGetPolicy<SyncPolicyGlibmm::Mutex> { typedef SyncPolicyGlibmm type; };
template<> struct SyncGetPolicy<SyncPolicyGlibmm::RecMutex> { typedef SyncPolicyGlibmm type; };
template<> struct SyncGetPolicy<SyncPolicyGlibmm::RWMutex> { typedef SyncPolicyGlibmm type; };






}  // ns




#endif
