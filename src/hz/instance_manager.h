/******************************************************************************
License: Zlib
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_INSTANCE_MANAGER_H
#define HZ_INSTANCE_MANAGER_H

#include <memory>



namespace hz {


/// Inherit this class template to have a single- or multi-instance objects, e.g. windows.
/// This is a multi-instance implementation.
template<class Child, bool MultiInstance>
class InstanceManager {
	protected:

		/// Can't construct / delete this directly! use create() and destroy()
		InstanceManager() = default;


	public:

		/// Deleted
		InstanceManager(const InstanceManager& other) = delete;

		/// Deleted
		InstanceManager(const InstanceManager&& other) = delete;

		/// Deleted
		InstanceManager& operator=(const InstanceManager&) = delete;

		/// Deleted
		InstanceManager& operator=(const InstanceManager&&) = delete;

		/// Default
		~InstanceManager() = default;


		/// The default multi-instance implementation doesn't support `instance()`
		static Child* instance() = delete;

};




/// Single-instance specialization
template<class Child>
class InstanceManager<Child, false> {
	protected:

		/// Can't construct / delete this directly! use create() and destroy()
		InstanceManager() = default;


	public:

		/// Deleted
		InstanceManager(const InstanceManager& other) = delete;

		/// Deleted
		InstanceManager(const InstanceManager&& other) = delete;

		/// Deleted
		InstanceManager& operator=(const InstanceManager&) = delete;

		/// Deleted
		InstanceManager& operator=(const InstanceManager&&) = delete;

		/// Default
		~InstanceManager() = default;


		/// Return a single existing instance of this template instantiation.
		/// \return nullptr if no instances were created yet.
		static Child* instance()
		{
			return instance_.get();
		}


	protected:

		/// Set the instance.
		static void set_single_instance(Child* instance)
		{
			instance_.reset(instance);
		}


	private:

		static inline std::unique_ptr<Child> instance_;  ///< Single instance pointer

};





}  // ns




#endif

/// @}
