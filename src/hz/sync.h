/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_SYNC_H
#define HZ_SYNC_H

#include "hz_config.h"  // feature macros

#include <exception>
#include <string>

#include "noncopyable.h"
#include "exceptions.h"  // THROW_FATAL
#include "sync_part_get_policy.h"  // internal header, SyncGetPolicy struct.



/**
\file
Threading policies providing synchronization primitives
through wrapping existing thread libraries.

Note that this is NOT a full-blown threading library. It merely
allows the library writers to make their libraries thread-safe
by using these primitives.

Use SyncPolicyNone for "doing nothing" and SyncPolicyMtDefault
for default MT policy. The default is controlled through
HZ_SYNC_DEFAULT_POLICY_* macros (see below).



------------------------------ Policy API ------------------------------

Every policy class provides:

	Mutex Types:

		- Mutex type. Possibly a wrapper around native mutex type.
			May or may not be recursive. No guaranteed methods.
			Guaranteed freeing of resource when destroyed.
		- NativeMutex type. The underlying mutex type, if available.
			If not available, it's typedefed as Mutex. No guaranteed
			methods. Don't declare objects of this type - it is there solely
			for interaction with existing native mutexes.

		- RecMutex type. Same as Mutex, but recursive. No guaranteed
			methods. Guaranteed freeing of resource when destroyed.
		- NativeRecMutex type. Same as NativeMutex, but recursive.
			No guaranteed methods.

		- RWMutex type. A mutex which supports read / write locking.
			No guaranteed methods.
			Guaranteed freeing of resource when destroyed.
			Note: There is no recursion or lock escalation with RWLock.
			That is, locking it multiple times (read or write), or write-locking
			while holding a read-lock may work in some situations with some
			backends, but can also cause a deadlock.
		- NativeRWMutex type. A cross between NativeMutex and RWMutex.

		The Native types are needed to give an explicit guarantee that these
		types are lockable via respective ScopedLock classes and static methods.


	Static Methods:

		bool init()\n
			Initialize the thread backend. The application MUST call this function
			prior to locking any mutexes from this policy. True is returned in case
			the backend is initialized successfully, or has been initialized already.
			Note that with some backends, failure cannot be trapped and the program
			may abort. If failure condition is detected, false is returned and
			sync_resource_error exception is thrown.

		The following static methods accept any mutex type declared above
		(including Native types).
		If preconditions aren't met, their behaviour is completely undefined
		(silent failure, hang, thrown exception - any one of these may be observed).
		In some cases, errors may be detected (not guaranteed though). If
		an error is detected, sync_resource_error exception is thrown. The
		reason for this is that the system is already in unstable state, so
		throwing a fatal exception seems to be the right choice.

		void lock(MutexType& m)\n
			Preconditions: m must not be locked by current thread.
			Locks the mutex via the mutex's defined locking facilities.
			This function will block until m is available for locking.

		bool trylock(MutexType& m)\n
			Preconditions: m must not be locked by current thread.
			Tries to lock the mutex via the mutex's defined locking facilities.
			This function will return false if lock could not be obtained, and
			true otherwise.

		void unlock(MutexType& m)\n
			Preconditions: m must be locked by current thread.
			Unlocks the previously locked mutex immediately.

		void lock(MutexType& m, bool for_write = false)\n
		bool trylock(MutexType& m, bool for_write = false)\n
		void unlock(MutexType& m, bool for_write = false)\n
			Same as above, but for RWMutex and NativeRWMutex types.
			The second parameter must be equal for any lock / unlock
			and trylock / unlock pair. If not, the behaviour is undefined.


	Lock Object Types:

		Caution: These types are NOT thread-safe, and they aren't meant
		to be passed around between threads.
		Note: We do not provide non-template typedefed names for these,
		because when dealing with unknown mutex types, the user cannot
		reliably determine the needed lock type. For example, the user cannot
		know if the supplied mutex is a simple mutex, or native recursive one.
		That's why it's best to avoid specific lock types altogether and use
		the templated ones. It's easy enough for the users to typedef them
		themselves if such need arises.
		In case of error, sync_resource_error is thrown. Note that this exception
		may be thrown from constructor. In that case, destruction is still necessary.

		template\<typename MutexType\>\n
		class GenericScopedLock\n
			Constructor:\n
				GenericScopedLock(MutexType& mutex, bool do_lock = true)\n
			Guaranteed to accept Mutex, NativeMutex, RecMutex,
			NativeRecMutex types.
			If do_lock is true, locks mutex when constructed and unlocks when
			destroyed. If do_lock is false, no operations are performed.
			Uses policy's static methods lock() and unlock() for doing it.

		template\<typename MutexType\>\n
		class GenericScopedTryLock\n
			Constructor:\n
				GenericScopedTryLock(MutexType& mutex, bool do_lock = true)\n
			Guaranteed to accept Mutex, NativeMutex, RecMutex,
			NativeRecMutex types.
			Behaves like ScopedTryLock, but provides an additional
			bool() operator, which returns false if do_lock was true and locking
			couldn't be performed. Also provides retry() method, which retries
			the operation (the lock must not be obtained at that point yet).
			Uses policy's static methods trylock() and unlock() for doing it.

		class GenericScopedRecLock, class GenericScopedRecTryLock\n
			There are no such types, use GenericScopedLock and
			GenericScopedTryLock instead. They accept recursive mutex types
			too. This is done to avoid requiring knowledge of the exact mutex
			type (non-recursive or recursive, non-native or native) when locking them.

		template\<typename MutexType\>\n
		class GenericScopedRWLock\n
			Constructor:\n
				ScopedRWLock(MutexType& mutex, bool for_write = false, bool do_lock = true)\n
			Guaranteed to accept RWMutex, NativeRWMutex types.
			Behaves similarily to ScopedLock, but works with read / write
			mutexes instead.

		template\<typename MutexType\>\n
		class GenericScopedRWTryLock\n
			Constructor:\n
				ScopedRWTryLock(MutexType& mutex, bool for_write = false, bool do_lock = true)\n
			A cross between ScopedRWLock and ScopedTryLock.

		ScopedLock, ScopedTryLock, ScopedNativeLock, ScopedNativeTryLock,
		ScopedRecLock, ScopedRecTryLock, ScopedNativeRecLock, ScopedNativeRecTryLock,
		ScopedRWLock, ScopedRWTryLock, ScopedNativeRWLock, ScopedNativeRWTryLock\n
			These are typedefs for respective (mutex, generic scoped lock) pairs.


Out-of-policy Class templates:

	template\<typename MutexType\>\n
	struct SyncGetPolicy { typedef unspecified_policy_type type; };\n
		This struct provides a way to retrieve a policy class type by supplying
		mutex type. Any mutex type is supported, provided that such mutex type
		exists in a policy somewhere.
		For example, to do a locking in a function template where the exact
		policy or mutex type is unknown, one could write:
\code
		template<typename MutexType>
		void f(MutexType& m)
		{
			typename SyncGetPolicy<MutexType>::type::ScopedLock lock(m);
			// ...
			// lock is released at the end of scope.
		}
\endcode
		This also provides safe defaults for class templates which accept mutexes
		as their template parameters, for example:
\code
		template<typename MutexType,
			typename Policy = typename SyncGetPolicy<MutexType>::type>
		class C {
			// use Policy::ScopedLock, etc...
		};
\endcode
*/




namespace hz {



/// This is thrown in case of really bad problems
struct sync_resource_error : virtual public std::exception {  // from <exception>

	/// Constructor
	sync_resource_error(const std::string& why) : why_(why)
	{ }

	/// Virtual destructor
	virtual ~sync_resource_error() throw() { }

	// Reimplemented from std::exception
 	virtual const char* what() const throw()
	{
		msg = "hz::sync_resource_error: " + why_;
		return msg.c_str();
	}

	// yes, they should not be strings, to avoid memory allocations.
	// no, I don't know how to return const char* without memory allocations.
	const std::string why_;  ///< The reason.
	mutable std::string msg;  ///< The message (used in implementation of what()). This must be a member to avoid its destruction on function call return. use what().
};




// -------------------------------- Helpers



// This class provides commonly used classes for policies.
template<typename Policy>
struct SyncScopedLockProvider {

	template<typename MutexType>
	class GenericScopedLock : public hz::noncopyable {
		public:

			GenericScopedLock(MutexType& mutex, bool do_lock = true)
					: mutex_(mutex), do_lock_(do_lock)
			{
				if (do_lock_)
					Policy::lock(mutex_);
			}

			~GenericScopedLock()
			{
				if (do_lock_)
					Policy::unlock(mutex_);
			}

		private:
			MutexType& mutex_;
			bool do_lock_;
	};


	template<typename MutexType>
	class GenericScopedTryLock : public hz::noncopyable {
		private:
			// Pointer to member function, used in bool() overloading.
			// We use this instead of bool to avoid its promotion to integer.
			// Pointers aren't promoted.
			typedef bool (GenericScopedTryLock::*unspecified_bool_type)() const;

		public:

			GenericScopedTryLock(MutexType& mutex, bool do_lock = true)
					: mutex_(mutex), do_lock_(do_lock), locked_(false)
			{
				if (do_lock_)
					locked_ = Policy::trylock(mutex_);
			}

			~GenericScopedTryLock()
			{
				if (do_lock_ && locked_)
					Policy::unlock(mutex_);
			}

			// for repeating the lock request
			bool retry()
			{
				if (locked_) {
					THROW_FATAL(sync_resource_error("GenericScopedTryLock::trylock(): Attempting to lock an already locked mutex."));
					return false;
				}
				if (do_lock_)
					return (locked_= Policy::trylock(mutex_));
				return true;  // it's always success if do_lock was false.
			}

			// This returns whether the operation could not be considered a success.
			// Not strictly required for bool conversion, but nice to have anyway.
			bool operator!() const
			{
				return !do_lock_ || locked_;  // if we're not locking, or if we're locking but it's not locked.
			}

			// aka operator bool().
			operator unspecified_bool_type() const
			{
				// in this case, &operator!, being a valid pointer-to-member-function,
				// serves as a "true" value, while 0 (being a null-pointer) serves as false.
				return (operator!() ? &GenericScopedTryLock::operator! : 0);
			}

		private:
			MutexType& mutex_;
			bool do_lock_;
			bool locked_;
	};


	template<typename MutexType>
	class GenericScopedRWLock : public hz::noncopyable {
		public:

			GenericScopedRWLock(MutexType& mutex, bool for_write = false, bool do_lock = true)
					: mutex_(mutex), do_lock_(do_lock), for_write_(for_write)
			{
				if (do_lock_)
					Policy::lock(mutex_, for_write_);
			}

			~GenericScopedRWLock()
			{
				if (do_lock_)
					Policy::unlock(mutex_, for_write_);
			}

		private:
			MutexType& mutex_;
			bool do_lock_;
			bool for_write_;
	};


	template<typename MutexType>
	class GenericScopedRWTryLock : public hz::noncopyable {
		private:
			typedef bool (GenericScopedRWTryLock::*unspecified_bool_type)() const;

		public:

			GenericScopedRWTryLock(MutexType& mutex, bool for_write = false, bool do_lock = true)
					: mutex_(mutex), do_lock_(do_lock), for_write_(for_write), locked_(false)
			{
				if (do_lock_)
					locked_ = Policy::trylock(mutex_, for_write);
			}

			~GenericScopedRWTryLock()
			{
				if (do_lock_ && locked_)
					Policy::unlock(mutex_, for_write_);
			}

			// for repeating the lock request with the same options as in constructor
			bool retry()
			{
				if (locked_)  // it's a fatal error, so no point in returning anything.
					THROW_FATAL(sync_resource_error("GenericScopedRWTryLock::trylock(): Attempting to lock an already locked mutex."));
				if (do_lock_)
					return (locked_= Policy::trylock(mutex_, for_write_));
				return true;  // it's always success if do_lock was false.
			}

			// This returns whether the operation could not be considered a success.
			// Not strictly required for bool conversion, but nice to have anyway.
			bool operator!() const
			{
				return do_lock_ && !locked_;
			}

			// aka operator bool().
			operator unspecified_bool_type() const
			{
				// in this case, &operator!, being a valid pointer-to-member-function,
				// serves as a "true" value, while 0 (being a null-pointer) serves as false.
				return (operator!() ? 0 : &GenericScopedRWTryLock::operator!);
			}

		private:
			MutexType& mutex_;
			bool do_lock_;
			bool for_write_;
			bool locked_;
	};

};



/// A type useful for dummy mutexes, etc...
/// TypeChanger may be used to generate different (non-inter-convertible)
/// types, for e.g. function overloading.
template<int TypeChanger>
struct SyncEmptyType { };



/// Classes may use this in single-threaded or non-locking environments.
struct SyncPolicyNone {
	// We use different types here to avoid user errors such as mixing
	// types when developing with single-threaded version.

	typedef SyncEmptyType<1> Mutex;
	typedef SyncEmptyType<2> NativeMutex;
	typedef SyncEmptyType<3> RecMutex;
	typedef SyncEmptyType<4> NativeRecMutex;
	typedef SyncEmptyType<5> RWMutex;
	typedef SyncEmptyType<6> NativeRWMutex;

	static bool init() { return true; }

	static void lock(Mutex& m) { }
	static bool trylock(Mutex& m) { return true; }  // behave as it was successful
	static void unlock(Mutex& m) { }

	static void lock(NativeMutex& m) { }
	static bool trylock(NativeMutex& m) { return true; }
	static void unlock(NativeMutex& m) { }

	static void lock(RecMutex& m) { }
	static bool trylock(RecMutex& m) { return true; }
	static void unlock(RecMutex& m) { }

	static void lock(NativeRecMutex& m) { }
	static bool trylock(NativeRecMutex& m) { return true; }
	static void unlock(NativeRecMutex& m) { }

	static void lock(RWMutex& m, bool for_write = false) { }
	static bool trylock(RWMutex& m, bool for_write = false) { return true; }
	static void unlock(RWMutex& m, bool for_write = false) { }

	static void lock(NativeRWMutex& m, bool for_write = false) { }
	static bool trylock(NativeRWMutex& m, bool for_write = false) { return true; }
	static void unlock(NativeRWMutex& m, bool for_write = false) { }


	/// Dummy scoped lock, does absolutely nothing. Works with all the mutex types.
	template<typename MutexType>
	class GenericScopedLock : public hz::noncopyable {
		public:
			GenericScopedLock(MutexType& mutex, bool do_lock = true)
			{ }

			~GenericScopedLock()
			{ }
	};


	template<typename MutexType>
	class GenericScopedTryLock : public hz::noncopyable {
		private:
			typedef bool (GenericScopedTryLock::*unspecified_bool_type)() const;

		public:
			GenericScopedTryLock(MutexType& mutex, bool do_lock = true) : do_lock_(do_lock)
			{ }

			~GenericScopedTryLock()
			{ }

			bool retry()
			{
				if (do_lock_)  // assume that constructor locked it successfully (api-wise).
					THROW_FATAL(sync_resource_error("GenericScopedTryLock::trylock(): Attempting to lock an already locked mutex."));
				return true;
			}

			bool operator!() const
			{
				return false;  // always success, which translates to false for this operator.
			}

			operator unspecified_bool_type() const
			{
				// in this case, &operator!, being a valid pointer-to-member-function,
				// serves as a "true" value, while 0 (being a null-pointer) serves as false.
				return &GenericScopedTryLock::operator!;  // always success
			}

		private:
			bool do_lock_;
	};


	template<typename MutexType>
	class GenericScopedRWLock : public hz::noncopyable {
		public:
			GenericScopedRWLock(MutexType& mutex, bool for_write = false, bool do_lock = true)
			{ }
			~GenericScopedRWLock()
			{ }
	};


	template<typename MutexType>
	class GenericScopedRWTryLock : public hz::noncopyable {
		private:
			typedef bool (GenericScopedRWTryLock::*unspecified_bool_type)() const;

		public:
			GenericScopedRWTryLock(MutexType& mutex, bool for_write = false, bool do_lock = true) : do_lock_(do_lock)
			{ }

			~GenericScopedRWTryLock()
			{ }

			bool retry()
			{
				if (do_lock_)  // assume that constructor locked it successfully (api-wise).
					THROW_FATAL(sync_resource_error("GenericScopedRWTryLock::trylock(): Attempting to lock an already locked mutex."));
				return true;
			}

			bool operator!() const
			{
				return false;  // always success, which translates to false for this operator.
			}

			operator unspecified_bool_type() const
			{
				// in this case, &operator!, being a valid pointer-to-member-function,
				// serves as a "true" value, while 0 (being a null-pointer) serves as false.
				return &GenericScopedRWTryLock::operator!;  // always success
			}

		private:
			bool do_lock_;
	};


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

};


// mutex -> policy

template<> struct SyncGetPolicy<SyncPolicyNone::Mutex> { typedef SyncPolicyNone type; };
template<> struct SyncGetPolicy<SyncPolicyNone::NativeMutex> { typedef SyncPolicyNone type; };
template<> struct SyncGetPolicy<SyncPolicyNone::RecMutex> { typedef SyncPolicyNone type; };
template<> struct SyncGetPolicy<SyncPolicyNone::NativeRecMutex> { typedef SyncPolicyNone type; };
template<> struct SyncGetPolicy<SyncPolicyNone::RWMutex> { typedef SyncPolicyNone type; };
template<> struct SyncGetPolicy<SyncPolicyNone::NativeRWMutex> { typedef SyncPolicyNone type; };





}  // ns



#if defined HZ_SYNC_DEFAULT_POLICY_GLIBMM
	#include "sync_policy_glibmm.h"
	namespace hz {
		typedef SyncPolicyGlibmm SyncPolicyMtDefault;
	}

#elif defined HZ_SYNC_DEFAULT_POLICY_GLIB
	#include "sync_policy_glib.h"
	namespace hz {
		typedef SyncPolicyGlib SyncPolicyMtDefault;
	}

#elif defined HZ_SYNC_DEFAULT_POLICY_BOOST
	#include "sync_policy_boost.h"
	namespace hz {
		typedef SyncPolicyBoost SyncPolicyMtDefault;
	}

#elif defined HZ_SYNC_DEFAULT_POLICY_POCO
	#include "sync_policy_poco.h"
	namespace hz {
		typedef SyncPolicyPoco SyncPolicyMtDefault;
	}

#elif defined HZ_SYNC_DEFAULT_POLICY_PTHREAD
	#include "sync_policy_pthread.h"
	namespace hz {
		typedef SyncPolicyPthread SyncPolicyMtDefault;
	}

#elif defined HZ_SYNC_DEFAULT_POLICY_WIN32
	#include "sync_policy_win32.h"
	namespace hz {
		typedef SyncPolicyWin32 SyncPolicyMtDefault;
	}

#else  // default: NONE
// #elif defined HZ_SYNC_DEFAULT_POLICY_NONE  // default:
	namespace hz {
		typedef SyncPolicyNone SyncPolicyMtDefault;
	}

#endif






#endif

/// @}
