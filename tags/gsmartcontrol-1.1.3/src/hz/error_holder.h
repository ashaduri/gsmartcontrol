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

#ifndef HZ_ERROR_HOLDER_H
#define HZ_ERROR_HOLDER_H

#include "hz_config.h"  // feature macros

#include <vector>

#include "ptr_container.h"
#include "sync.h"
#include "sync_multilock.h"  // SyncMultiLock
#include "error.h"
#include "debug.h"



namespace hz {



/// A class wishing to implement Error holding storage should inherit this.
/// Unless specified otherwise, all methods are thread-safe if the
/// thread-safe locking policy is provided.
/// \tparam LockPolicy_ A class with methods to lock / unlock current object access
/// for use in multi-threaded environments. See sync.h for more info.
template <class LockPolicy_>
class ErrorHolder {
	public:

		typedef std::vector<ErrorBase*> error_list_t;  ///< A list of ErrorBase* pointers
		typedef ptr_container<error_list_t> ptr_error_list_t;  ///< A list of auto-deleted pointers to error_list_t

		typedef LockPolicy_ ErrorLockPolicy;  ///< Locking policy
		typedef typename ErrorLockPolicy::ScopedLock ErrorScopedLock;  ///< Scoped lock class

		/// Make it its own friend for mutex visibility
		template<class T> friend class ErrorHolder;

		/// Virtual destructor
		virtual ~ErrorHolder()
		{ }


		/// Add an error to the error list
		template<class E>
		void push_error(const E& e, bool do_lock = true)
		{
			ErrorScopedLock locker(error_object_mutex_, do_lock);
			E* copied = new E(e);  // clone e. it will be deleted when the list is destroyed.
			errors_.push_back(copied);

			error_warn(copied);
		}


		/// Remove last error from the error list
		template<class E>
		void pop_last_error(E& e, bool do_lock = true)
		{
			ErrorScopedLock locker(error_object_mutex_, do_lock);
			e = static_cast<E>(errors_.pop_back());
		}


		/// Import errors from another object
		template<class T>
		void import_errors(ErrorHolder<T>& other, bool do_lock_this = true, bool do_lock_other = true)
		{
		    if (this == reinterpret_cast<ErrorHolder*>(&other))
        		return;

			// always maintain the same locking order to avoid deadlocks.

/*			if (&error_object_mutex_ < reinterpret_cast<typename ErrorLockPolicy::Mutex*>(&(other.error_object_mutex_))) {
				if (do_lock_this)
					ErrorLockPolicy::lock(error_object_mutex_);
				if (do_lock_other)
					ErrorHolder<T>::ErrorLockPolicy::lock(other.error_object_mutex_);
			} else {
				if (do_lock_other)
					ErrorHolder<T>::ErrorLockPolicy::lock(other.error_object_mutex_);
				if (do_lock_this)
					ErrorLockPolicy::lock(error_object_mutex_);
			}
*/

			SyncMultiLock<typename ErrorLockPolicy::Mutex, typename ErrorHolder<T>::ErrorLockPolicy::Mutex>
			locker(error_object_mutex_, other.error_object_mutex_, do_lock_this, do_lock_other);

			error_list_t tmp;  // non-pointer-basec container
			other.get_errors_cloned(tmp, false);
			errors_.insert(errors_.end(), tmp.begin(), tmp.end());  // copies the deletion ownership too

/*
			if (&error_object_mutex_ < reinterpret_cast<typename ErrorLockPolicy::Mutex*>(&(other.error_object_mutex_))) {
				if (do_lock_this)
					ErrorLockPolicy::unlock(error_object_mutex_);
				if (do_lock_other)
					ErrorHolder<T>::ErrorLockPolicy::unlock(other.error_object_mutex_);
			} else {
				if (do_lock_other)
					ErrorHolder<T>::ErrorLockPolicy::unlock(other.error_object_mutex_);
				if (do_lock_this)
					ErrorLockPolicy::unlock(error_object_mutex_);
			}
*/
		}


		/// Check if there are any errors in this class.
		bool has_errors(bool do_lock = true) const
		{
			ErrorScopedLock locker(error_object_mutex_, do_lock);
			return !errors_.empty();
		}


		/// Get a list of errors.
		/// NOTE: You MUST do additional locking (and possibly pass do_lock=false here)
		/// if you intend to use the elements of the returned array. If you don't do that,
		/// this class may just delete its pointers and you'll be left with dangling pointers.
		error_list_t get_errors(bool do_lock = true) const
		{
			ErrorScopedLock locker(error_object_mutex_, do_lock);
			// copy the pointers to deque, return it.
			error_list_t ret(errors_.begin(), errors_.end());
			return ret;
		}


		/// Get a cloned list of errors.
		/// You MUST delete the elements of the returned container!
		template<class ReturnedContainer>
		void get_errors_cloned(ReturnedContainer& put_here, bool do_lock = true) const
		{
			ErrorScopedLock locker(error_object_mutex_, do_lock);
			errors_.clone_by_method_to(put_here);  // calls clone() on each element
		}


		// These functions are the recommended way to access the errors container.

		/// A begin() function for the error list.
		error_list_t::iterator errors_begin(bool do_lock = true)
		{
			ErrorScopedLock locker(error_object_mutex_, do_lock);
			return errors_.begin();
		}

		/// A begin() function for the error list (const version).
		error_list_t::const_iterator errors_begin(bool do_lock = true) const
		{
			ErrorScopedLock locker(error_object_mutex_, do_lock);
			return errors_.begin();
		}

		/// An end() function for the error list.
		error_list_t::iterator errors_end(bool do_lock = true)
		{
			ErrorScopedLock locker(error_object_mutex_, do_lock);
			return errors_.end();
		}

		/// An end() function for the error list (const version).
		error_list_t::const_iterator errors_end(bool do_lock = true) const
		{
			ErrorScopedLock locker(error_object_mutex_, do_lock);
			return errors_.end();
		}


		/// Clear the error list
		void clear_errors(bool do_lock = true)
		{
			ErrorScopedLock locker(error_object_mutex_, do_lock);
			errors_.clear();
		}


		/// Lock the error list
		void errors_lock()
		{
			ErrorLockPolicy::lock(error_object_mutex_);
		}


		/// Unlock the error list
		void errors_unlock()
		{
			ErrorLockPolicy::unlock(error_object_mutex_);
		}


		/// This function is called every time push_error() is invoked.
		/// The default implementation prints the message using libdebug.
		/// Override in children if needed.
		virtual void error_warn(ErrorBase* e)
		{
			std::string msg = e->get_type() + ": " + e->get_message() + "\n";
			ErrorLevel::level_t level = e->get_level();

			// use debug macros, not functions (to allow complete removal through preprocessor).
			switch (level) {
				case ErrorLevel::none: break;
				case ErrorLevel::dump: debug_out_dump("hz", msg); break;
				case ErrorLevel::info: debug_out_info("hz", msg); break;
				case ErrorLevel::warn: debug_out_warn("hz", "Warning: " << msg); break;
				case ErrorLevel::error: debug_out_error("hz", "Error: " << msg); break;
				case ErrorLevel::fatal: debug_out_fatal("hz", "Fatal: " << msg); break;
			}
		}


	protected:

		ptr_error_list_t errors_;  ///< Error list. The newest errors at the end.

		mutable typename ErrorLockPolicy::Mutex error_object_mutex_;  ///< Mutex to protect multi-threaded access.

};



/// An error holder with no locking.
typedef ErrorHolder<SyncPolicyNone> ErrorHolderST;

/// An error holder that does its own locking through mutexes.
typedef ErrorHolder<SyncPolicyMtDefault> ErrorHolderMT;





}  // ns




#endif

/// @}
