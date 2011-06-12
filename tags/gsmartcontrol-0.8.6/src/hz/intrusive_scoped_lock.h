/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt
***************************************************************************/

#ifndef HZ_INTRUSIVE_SCOPED_LOCK_H
#define HZ_INTRUSIVE_SCOPED_LOCK_H

#include "hz_config.h"  // feature macros

/*
Intrusive scoped lock class.
The locking will be done by calling lock() and unlock()
methods of parameter object.

LockingPolicy template parameter may be used to control
which functions are called, as well as how.


Example usage:

{
	SharedData* sd = get_some_shared_data();  // SharedData must have lock() and unlock().
	intrusive_scoped_lock<SharedData*> locker(sd);  // this will invoke sd->lock()

	// at the end of the scope, sd->unlock() will be called.
}

*/


namespace hz {



// The template parameter class is expected to have
// lock() and unlock() functions.
template <class T>
struct IntrusiveLockingPolicyDefault {
	static void lock(T& obj)
	{
		obj.lock();
	}

	static void unlock(T& obj)
	{
		obj.unlock();
	}
};


// specialization for pointers
template <class T>
struct IntrusiveLockingPolicyDefault<T*> {
	static void lock(T obj)
	{
		obj->lock();
	}

	static void unlock(T obj)
	{
		obj->unlock();
	}
};



// Same as above, but call the lock through "->", not ".".
// This allows its usage with smart pointers which transfer
// their locking to pointee objects.
template <class T>
struct IntrusiveLockingPolicySmart {

	static void lock(T& obj)
	{
		(&obj)->lock();
	}

	static void unlock(T& obj)
	{
		(&obj)->unlock();
	}

};



// "don't do any actual locking" policy
template <class T>
struct IntrusiveLockingPolicyNone {

	static void lock(T& obj)
	{
	}

	static void unlock(T& obj)
	{
	}

};



template <class T, class LockingPolicy = IntrusiveLockingPolicyDefault<T> >
class intrusive_scoped_lock {

	public:

		intrusive_scoped_lock(T& r, bool do_lock_ = true) : ref(r), do_lock(do_lock_)
		{
			if (do_lock)
				LockingPolicy::lock(ref);
		}

		~intrusive_scoped_lock()
		{
			if (do_lock)
				LockingPolicy::unlock(ref);
		}

		T& get()
		{
			return ref;
		}


	private:

		// disallow copying

		intrusive_scoped_lock& operator=(const intrusive_scoped_lock&);

		intrusive_scoped_lock(const intrusive_scoped_lock&);


		T& ref;

		bool do_lock;

};






}  // ns



#endif
