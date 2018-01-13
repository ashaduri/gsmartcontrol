/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_INSTANCE_MANAGER_H
#define HZ_INSTANCE_MANAGER_H

#include "hz_config.h"  // feature macros



namespace hz {


/**
Inherit this class to have a single- or multi-instance objects, e.g. windows.
*/
template<class Child, bool MultiInstance>
class InstanceManager {
	protected:

		/// Can't construct / delete this directly! use create() and destroy()
		InstanceManager() = default;

		/// Virtual destructor (protected, can't delete this directly, use destroy()).
		virtual ~InstanceManager() = default;

		/// Non-construction-copyable
		InstanceManager(const InstanceManager& other) = delete;

		/// Non-copyable
		InstanceManager& operator=(const InstanceManager&) = delete;


	public:

		/// Create a new instance or return an already created one if single-instance.
		/// If single-instance, the call will be serialized.
		static Child* create()
		{
			Child* o = new Child;
			o->obj_create();
			return o;
		}


		/// Destroy an instance. \c instance must be passed if using
		/// multi-instance object. If single-instance, \c instance has no effect.
		/// If single-instance, the call will be serialized.
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

		/// Called from create(), right after constructor.
		virtual void obj_create() { }

		/// Called from destroy(), right before destructor.
		virtual void obj_destroy() { }


		// We have these functions for multi-instance variant too to
		// support transparently switching between them.

		/// Returns true if there is a valid single-instance object.
		/// In multi-instance version this always returns false.
		static bool has_single_instance()
		{
			return false;
		}

		/// Returns the instance if there is a valid single-instance one.
		/// In multi-instance version this always returns 0.
		static Child* get_single_instance()
		{
			return 0;
		}

		/// Set single-instance object.
		/// This function has no effect in multi-instance version.
		static void set_single_instance(Child* instance)
		{ }

};




/// Single-instance specialization
template<class Child>
class InstanceManager<Child, false> {
	protected:

		InstanceManager() = default;

		virtual ~InstanceManager() = default;

		/// Non-construction-copyable
		InstanceManager(const InstanceManager& other) = delete;

		/// Non-copyable
		InstanceManager& operator=(const InstanceManager&) = delete;


	public:

		static Child* create()
		{
			if (instance_)  // for single-instance objects
				return instance_;

			Child* o = new Child;
			o->obj_create();

			instance_ = o;  // for single-instance objects
			return o;
		}


		static void destroy(Child* instance = 0)
		{
			if (instance_) {
				instance_->obj_destroy();
				delete instance_;
				instance_ = 0;
			}
		}


	protected:

		virtual void obj_create() { }

		virtual void obj_destroy() { }


		static bool has_single_instance()
		{
			return instance_;
		}

		static Child* get_single_instance()
		{
			return instance_;
		}

		static void set_single_instance(Child* instance)
		{
			instance_ = instance;
		}


		// if an object is allowed to have a single instance only, these are needed.
		static Child* instance_;  ///< Single instance pointer

// 		static const bool multi_instance_ = false;

};




/// Single instance pointer.
/// This is a definition of class template's static member variable
/// (it can be in a header (as opposed to cpp) if it's class template member).
/// This will generate different instances as long as the class type is different.
/// By passing the child's class type as template parameter we guarantee that.
template<class Child>
Child* InstanceManager<Child, false>::instance_ = 0;







}  // ns




#endif

/// @}
