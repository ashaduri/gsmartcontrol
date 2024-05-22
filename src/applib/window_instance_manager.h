/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef WINDOW_INSTANCE_MANAGER_H
#define WINDOW_INSTANCE_MANAGER_H

#include <memory>
#include <unordered_set>
#include <glibmm.h>
#include <gtkmm.h>



class WindowInstanceManagerStorage {
	public:

		/// Store an instance and keep it alive.
		/// Return a newly stored shared pointer to the instance.
		static std::shared_ptr<Gtk::Window> store_instance(Gtk::Window* obj)
		{
			std::shared_ptr<Gtk::Window> obj_sptr(obj);
			instances_.insert(obj_sptr);
			return obj_sptr;
		}


		/// Destroy a previously stored instance
		static void destroy_instance(Gtk::Window* window)
		{
			auto found = std::find_if(instances_.begin(), instances_.end(), [window](const std::shared_ptr<Gtk::Window>& elem) { return elem.get() == window; });
			if (found != instances_.end()) {
				instances_.erase(found);
			}
		}


		/// Destroy all stored instances
		static void destroy_all_instances()
		{
			instances_.clear();
		}


	private:
		/// All instances of created objects, kept alive by shared_ptr
		static inline std::unordered_set<std::shared_ptr<Gtk::Window>> instances_;

};



/// Inherit this class template to have a single- or multi-instance objects, e.g. windows.
/// This is a multi-instance implementation.
template<class Child, bool MultiInstance>
class WindowInstanceManager {
	protected:

		/// Can't construct / delete this directly! use create() and destroy()
		WindowInstanceManager() = default;


	public:

		/// Deleted
		WindowInstanceManager(const WindowInstanceManager& other) = delete;

		/// Deleted
		WindowInstanceManager(WindowInstanceManager&& other) = delete;

		/// Deleted
		WindowInstanceManager& operator=(const WindowInstanceManager&) = delete;

		/// Deleted
		WindowInstanceManager& operator=(WindowInstanceManager&&) = delete;

		/// Default, must be polymorphic for casts to succeed
		virtual ~WindowInstanceManager() = default;


		/// The default multi-instance implementation doesn't support `instance()`
		static Child* instance() = delete;


		/// Destroy a previously stored instance
		void destroy_instance()
		{
			WindowInstanceManagerStorage::destroy_instance(dynamic_cast<Gtk::Window*>(this));  // side-cast
		}


	protected:

		/// Store an instance and keep it alive.
		/// Return a newly stored shared pointer to the instance.
		static std::shared_ptr<Child> store_instance(Child* obj)
		{
			return std::dynamic_pointer_cast<Child>(WindowInstanceManagerStorage::store_instance(obj));
		}

};




/// Single-instance specialization. This deletes the instance on program exit.
template<class Child>
class WindowInstanceManager<Child, false> {
	protected:

		/// Can't construct / delete this directly! use create() and destroy()
		WindowInstanceManager() = default;


	public:

		/// Deleted
		WindowInstanceManager(const WindowInstanceManager& other) = delete;

		/// Deleted
		WindowInstanceManager(WindowInstanceManager&& other) = delete;

		/// Deleted
		WindowInstanceManager& operator=(const WindowInstanceManager&) = delete;

		/// Deleted
		WindowInstanceManager& operator=(WindowInstanceManager&&) = delete;

		/// Default, must be polymorphic for casts to succeed
		virtual ~WindowInstanceManager() = default;


		/// Return a single existing instance of this template instantiation.
		/// \return nullptr if no instances were created yet.
		static std::shared_ptr<Child> instance()
		{
			return instance_.lock();
		}


		/// Destroy a previously stored instance
		void destroy_instance()
		{
			WindowInstanceManagerStorage::destroy_instance(dynamic_cast<Gtk::Window*>(this));  // side-cast
		}


	protected:

		/// Store an instance and keep it alive.
		/// Return a newly stored shared pointer to the instance.
		static std::shared_ptr<Child> store_instance(Child* obj)
		{
			auto inst = std::dynamic_pointer_cast<Child>(WindowInstanceManagerStorage::store_instance(obj));
			instance_ = inst;
			return inst;
		}


	private:

		static inline std::weak_ptr<Child> instance_;  ///< Single instance pointer

};






#endif

/// @}
