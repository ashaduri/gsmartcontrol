/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_SYNC_POLICY_GLIB_H
#define HZ_SYNC_POLICY_GLIB_H

#include "hz_config.h"  // feature macros

#include <glib.h>

#include "sync.h"


// Glib-based policy.


// Note: g_static_mutex_*lock() functions may give warnings about breaking strict-aliasing rules.
// The warnings are completely harmless and visible on some versions of glib only.
// However, due to their number, I decided to implement this workaround.

#ifndef _WIN32
	// same as stock version, but an additional cast to (void*) is added.
	#define hz_glib_static_mutex_get_mutex(mutex) \
		( g_thread_use_default_impl ? ((GMutex*) ((void*)((mutex)->static_mutex.pad))) : \
		g_static_mutex_get_mutex_impl_shortcut(&((mutex)->runtime_mutex)) )

#else
	// win32 has different definition of this macro, so default to stock version.
	#define hz_glib_static_mutex_get_mutex(mutex) g_static_mutex_get_mutex(mutex)
#endif


#define hz_glib_static_mutex_lock(mutex) \
	g_mutex_lock(hz_glib_static_mutex_get_mutex(mutex))

#define hz_glib_static_mutex_trylock(mutex) \
	g_mutex_trylock(hz_glib_static_mutex_get_mutex(mutex))

#define hz_glib_static_mutex_unlock(mutex) \
	g_mutex_unlock(hz_glib_static_mutex_get_mutex(mutex))




namespace hz {



class MutexGlib : public hz::noncopyable {
	public:
		typedef GStaticMutex native_type;

		static void native_lock(native_type& mutex)
		{
			hz_glib_static_mutex_lock(&mutex);
		}

		static bool native_trylock(native_type& mutex)
		{
			return hz_glib_static_mutex_trylock(&mutex);
		}

		static void native_unlock(native_type& mutex)
		{
			hz_glib_static_mutex_unlock(&mutex);
		}

		MutexGlib() { g_static_mutex_init(&mutex_); }
		~MutexGlib() { g_static_mutex_free(&mutex_); }

		void lock() { native_lock(mutex_); }
		bool trylock() { return native_trylock(mutex_); }
		void unlock() { native_unlock(mutex_); }

	private:
		GStaticMutex mutex_;  // use StaticMutex, I think it uses less heap memory
};



class RecMutexGlib : public hz::noncopyable {
	public:
		typedef GStaticRecMutex native_type;

		static void native_lock(native_type& mutex)
		{
			g_static_rec_mutex_lock(&mutex);
		}

		static bool native_trylock(native_type& mutex)
		{
			return g_static_rec_mutex_trylock(&mutex);
		}

		static void native_unlock(native_type& mutex)
		{
			g_static_rec_mutex_unlock(&mutex);
		}

		RecMutexGlib() { g_static_rec_mutex_init(&mutex_); }
		~RecMutexGlib() { g_static_rec_mutex_free(&mutex_); }

		void lock() { native_lock(mutex_); }
		bool trylock() { return native_trylock(mutex_); }
		void unlock() { native_unlock(mutex_); }

	private:
		GStaticRecMutex mutex_;
};



class RWMutexGlib : public hz::noncopyable {
	public:
		typedef GStaticRWLock native_type;

		static void native_lock(native_type& mutex, bool for_write = false)
		{
			if (for_write) {
				g_static_rw_lock_writer_lock(&mutex);
			} else {
				g_static_rw_lock_reader_lock(&mutex);
			}
		}

		static bool native_trylock(native_type& mutex, bool for_write = false)
		{
			return (for_write ? g_static_rw_lock_writer_trylock(&mutex) : g_static_rw_lock_reader_trylock(&mutex));
		}

		static void native_unlock(native_type& mutex, bool for_write = false)
		{
			if (for_write) {
				g_static_rw_lock_writer_unlock(&mutex);
			} else {
				g_static_rw_lock_reader_unlock(&mutex);
			}
		}

		RWMutexGlib() { g_static_rw_lock_init(&mutex_); }
		~RWMutexGlib() { g_static_rw_lock_free(&mutex_); }

		void lock(bool for_write = false) { native_lock(mutex_, for_write); }
		bool trylock(bool for_write = false) { return native_trylock(mutex_, for_write); }
		void unlock(bool for_write = false) { native_unlock(mutex_, for_write); }

	private:
		GStaticRWLock mutex_;
};




// Native type notes:
// MutexGlib's underlying structure is neither guaranteed to be
// recursive nor to be non-recursive.
// All these mutexes won't do anything unless gthread is initialized.

// Note: For this policy to work, you MUST initialize GThread system
// by calling one of the following (doesn't matter which exactly) beforehand:
// Glib::thread_init();  // glibmm
// or
// g_thread_init(NULL);  // glib
// or
// SyncPolicyGlib::init();  // sync's wrapper

// If you don't do it, the operations will silently do nothing.


// Glib doesn't provide any means to detect errors, so no exceptions here.

struct SyncPolicyGlib : public SyncScopedLockProvider<SyncPolicyGlib> {

	// Types:

	typedef MutexGlib Mutex;
	typedef Mutex::native_type NativeMutex;

	typedef RecMutexGlib RecMutex;
	typedef RecMutex::native_type NativeRecMutex;

	typedef RWMutexGlib RWMutex;
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

	// If threads are unavailable, this will abort.
	static bool init() { if (!g_thread_supported()) g_thread_init(NULL); return true; }

	static void lock(Mutex& m) { m.lock(); }
	static bool trylock(Mutex& m) { return m.trylock(); }
	static void unlock(Mutex& m) { m.unlock(); }

	static void lock(NativeMutex& m) { Mutex::native_lock(m); }
	static bool trylock(NativeMutex& m) { return Mutex::native_trylock(m); }
	static void unlock(NativeMutex& m) { Mutex::native_unlock(m); }

	static void lock(RecMutex& m) { m.lock(); }
	static bool trylock(RecMutex& m) { return m.trylock(); }
	static void unlock(RecMutex& m) { m.unlock(); }

	static void lock(NativeRecMutex& m) { RecMutex::native_lock(m); }
	static bool trylock(NativeRecMutex& m) { return RecMutex::native_trylock(m); }
	static void unlock(NativeRecMutex& m) { RecMutex::native_unlock(m); }

	static void lock(RWMutex& m, bool for_write = false) { m.lock(for_write); }
	static bool trylock(RWMutex& m, bool for_write = false) { return m.trylock(for_write); }
	static void unlock(RWMutex& m, bool for_write = false) { m.unlock(for_write); }

	static void lock(NativeRWMutex& m, bool for_write = false) { RWMutex::native_lock(m, for_write); }
	static bool trylock(NativeRWMutex& m, bool for_write = false) { return RWMutex::native_trylock(m, for_write); }
	static void unlock(NativeRWMutex& m, bool for_write = false) { RWMutex::native_unlock(m, for_write); }

};



// mutex -> policy

template<> struct SyncGetPolicy<SyncPolicyGlib::Mutex> { typedef SyncPolicyGlib type; };
template<> struct SyncGetPolicy<SyncPolicyGlib::NativeMutex> { typedef SyncPolicyGlib type; };
template<> struct SyncGetPolicy<SyncPolicyGlib::RecMutex> { typedef SyncPolicyGlib type; };
template<> struct SyncGetPolicy<SyncPolicyGlib::NativeRecMutex> { typedef SyncPolicyGlib type; };
template<> struct SyncGetPolicy<SyncPolicyGlib::RWMutex> { typedef SyncPolicyGlib type; };
template<> struct SyncGetPolicy<SyncPolicyGlib::NativeRWMutex> { typedef SyncPolicyGlib type; };




}  // ns




#endif
