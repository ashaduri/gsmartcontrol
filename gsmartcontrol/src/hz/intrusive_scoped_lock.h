/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_INTRUSIVE_SCOPED_LOCK_H
#define HZ_INTRUSIVE_SCOPED_LOCK_H

#include "hz_config.h"  // feature macros




namespace hz {



/// Locking policy for intrusive_scoped_lock. The template parameter
/// class is expected to have lock() and unlock() functions.
template <class T>
struct IntrusiveLockingPolicyDefault {
	/// Call obj.lock()
	static void lock(T& obj)
	{
		obj.lock();
	}

	/// Call obj.unlock()
	static void unlock(T& obj)
	{
		obj.unlock();
	}
};


/// Specialization for pointers
template <class T>
struct IntrusiveLockingPolicyDefault<T*> {
	/// Call obj->lock()
	static void lock(T obj)
	{
		obj->lock();
	}

	/// Call obj->unlock()
	static void unlock(T obj)
	{
		obj->unlock();
	}
};



/// Same as IntrusiveLockingPolicyDefault, but call the lock through
/// "->", not ".". This allows its usage with smart pointers which transfer
/// their locking to pointee objects.
template <class T>
struct IntrusiveLockingPolicySmart {
	/// Call (&obj)->lock()
	static void lock(T& obj)
	{
		(&obj)->lock();
	}

	/// Cal (&obj)->unlock()
	static void unlock(T& obj)
	{
		(&obj)->unlock();
	}

};



/// Locking policy for intrusive_scoped_lock which does no locking at all.
template <class T>
struct IntrusiveLockingPolicyNone {

	/// Do nothing
	static void lock(T& obj)
	{ }

	/// Do nothing
	static void unlock(T& obj)
	{ }

};



/**
Intrusive scoped lock class. The locking is done by calling lock() and unlock()
methods of parameter object.

LockingPolicy template parameter may be used to control which functions
are called, as well as how.

Example usage:
\code
{
	SharedData* sd = get_some_shared_data();  // SharedData must have lock() and unlock().
	intrusive_scoped_lock<SharedData*> locker(sd);  // this will invoke sd->lock()

	// at the end of the scope, sd->unlock() will be called.
}
\endcode
*/
template <class T, class LockingPolicy = IntrusiveLockingPolicyDefault<T> >
class intrusive_scoped_lock {
	public:

		/// Constructor, locks the object. Note that the object is stored by reference
		/// and therefore must exist as long as this object itself exists.
		intrusive_scoped_lock(T& r, bool do_lock_ = true) : ref(r), do_lock(do_lock_)
		{
			if (do_lock)
				LockingPolicy::lock(ref);
		}

		/// Destructor, unlocks the object.
		~intrusive_scoped_lock()
		{
			if (do_lock)
				LockingPolicy::unlock(ref);
		}

		/// Get the object we're operating with
		T& get()
		{
			return ref;
		}


	private:

		// Private assignment operator (disallow copying)
		intrusive_scoped_lock& operator=(const intrusive_scoped_lock&);

		// Private copy constructor (disallow copying)
		intrusive_scoped_lock(const intrusive_scoped_lock&);


		T& ref;  ///< The object we're operating with

		bool do_lock;  ///< If we're locking or not

};






}  // ns



#endif

/// @}
