/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
License: Zlib
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_INSTANCE_MANAGER_H
#define HZ_INSTANCE_MANAGER_H



namespace hz {


/**
Inherit this class to have a single- or multi-instance objects, e.g. windows.
This is a multi-instance specialization.
*/
template<class Child, bool MultiInstance>
class InstanceManager {
	protected:

		/// Can't construct / delete this directly! use create() and destroy()
		InstanceManager() = default;

		/// Can't construct / delete this directly! use create() and destroy()
		~InstanceManager() = default;


	public:

		/// Non-copyable
		InstanceManager(const InstanceManager& other) = delete;

		/// Non-copyable
		InstanceManager& operator=(const InstanceManager&) = delete;


		/// Create a new instance or return an already created one if single-instance.
		/// If single-instance, the call will be serialized.
		static Child* create()
		{
			return new Child();
		}


		/// Destroy an instance. \c instance must be passed if using
		/// multi-instance object. If single-instance, \c instance has no effect.
		/// If single-instance, the call will be serialized.
		static void destroy(Child* instance)
		{
			if (instance) {
				delete instance;
			}
		}


	protected:

		// We have these functions for multi-instance variant too to
		// support transparently switching between them.

		/// Returns true if there is a valid single-instance object.
		/// In multi-instance version this always returns false.
		static constexpr bool has_single_instance()
		{
			return false;
		}

};




/// Single-instance specialization
template<class Child>
class InstanceManager<Child, false> {
	protected:

		InstanceManager() = default;

		~InstanceManager() = default;


	public:

		/// Non-construction-copyable
		InstanceManager(const InstanceManager& other) = delete;

		/// Non-copyable
		InstanceManager& operator=(const InstanceManager&) = delete;


		static Child* create()
		{
			if (instance_)  // for single-instance objects
				return instance_;

			instance_ = new Child();
			return instance_;
		}


		static void destroy()
		{
			if (instance_) {
				delete instance_;
				instance_ = nullptr;
			}
		}


	protected:

		static constexpr bool has_single_instance()
		{
			return (bool)instance_;
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
		static inline Child* instance_ = nullptr;  ///< Single instance pointer

};





}  // ns




#endif

/// @}
