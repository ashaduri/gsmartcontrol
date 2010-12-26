/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_INSTANCE_MANAGER_H
#define HZ_INSTANCE_MANAGER_H

#include "hz_config.h"  // feature macros

#include "sync.h"  // SyncPolicyNone
#include "noncopyable.h"



namespace hz {


// Inherit from this class to generate a single- or multi-instance
// objects, e.g. windows.


// Single-instance variant uses locks to protect the data when
// manipulating it through its functions. If you intend to write to
// the pointed object and use it in separate threads, you will
// need to lock it manually.

// Multi-instance variant doesn't store any instances itself, so
// you are expected to perform any necessary locking with the
// pointers you hold.


// Single-instance specialization is defined below.
// SingleLockPolicy has no effect if MultiInstance is true.
template<class Child, bool MultiInstance, class SingleLockPolicy = SyncPolicyNone>
class InstanceManager : public hz::noncopyable {  // disallow copying

	protected:

		// can't construct / delete this directly! use create() and destroy()
		InstanceManager() { }

		virtual ~InstanceManager() { }


	public:

		// Create a new instance or return an already created one if single-instance.
		// If single-instance, the call will be serialized.
		static Child* create()
		{
			Child* o = new Child;
			o->obj_create();
			return o;
		}


		// Destroy an instance. "instance" parameter must be passed if using
		// multi-instance object. If single-instance, "instance" has no effect.
		// If single-instance, the call will be serialized.
		static void destroy(Child* instance = 0)
		{
			if (instance) {
				instance->obj_destroy();
				delete instance;
				// zeroify yourself
			}
		}


	protected:

		// These functions are called when the object instance is
		// created or destroyed through ::create() and ::destroy().

		// called from create(), right after constructor.
		virtual void obj_create() { }

		// called from destroy(), right before destructor.
		virtual void obj_destroy() { }


		// We have these functions for multi-instance variant too to
		// support transparently switching between them.

		// for single-instance objects only
		static bool has_single_instance(bool do_lock = true)
		{
			return false;
		}

		// for single-instance objects only
		static Child* get_single_instance(bool do_lock = true)
		{
			return 0;
		}

		// for single-instance objects only
		static void set_single_instance(Child* instance, bool do_lock = true)
		{ }

};




// Single-instance specialization
template<class Child, class SingleLockPolicy>
class InstanceManager<Child, false, SingleLockPolicy> : public hz::noncopyable {  // disallow copying

	protected:

		InstanceManager() { }

		virtual ~InstanceManager() { }


	public:

		static Child* create()
		{
			typename SingleLockPolicy::ScopedLock lock(instance_mutex_);

			if (instance_)  // for single-instance objects
				return instance_;

			Child* o = new Child;
			o->obj_create();

			instance_ = o;  // for single-instance objects
			return o;
		}


		static void destroy(Child* instance = 0)
		{
			typename SingleLockPolicy::ScopedLock lock(instance_mutex_);

			if (instance_) {
				instance_->obj_destroy();
				delete instance_;
				instance_ = 0;
			}
		}


	protected:

		// These functions are called when the object instance is
		// created or destroyed through ::create() and ::destroy().

		// called from create(), right after constructor.
		virtual void obj_create() { }

		// called from destroy(), right before destructor.
		virtual void obj_destroy() { }


		static bool has_single_instance(bool do_lock = true)
		{
			typename SingleLockPolicy::ScopedLock lock(instance_mutex_, do_lock);
			return instance_;
		}

		static Child* get_single_instance(bool do_lock = true)
		{
			typename SingleLockPolicy::ScopedLock lock(instance_mutex_, do_lock);
			return instance_;
		}

		static void set_single_instance(Child* instance, bool do_lock = true)
		{
			typename SingleLockPolicy::ScopedLock lock(instance_mutex_, do_lock);
			instance_ = instance;
		}


		// if an object is allowed to have a single instance only, these are needed.
		static Child* instance_;
		static typename SingleLockPolicy::Mutex instance_mutex_;

// 		static const bool multi_instance_ = false;

};




// Singleton instance (or whatever).
// This is a definition of class template's static member variable
// (it can be in a header (as opposed to cpp) if it's class template member).
// This will generate different instances as long as the class type is different.
// By passing the child's class type as template parameter we guarantee that.
template<class Child, class SingleLockPolicy>
Child* InstanceManager<Child, false, SingleLockPolicy>::instance_ = 0;

template<class Child, class SingleLockPolicy>
typename SingleLockPolicy::Mutex InstanceManager<Child, false, SingleLockPolicy>::instance_mutex_;







}  // ns




#endif
