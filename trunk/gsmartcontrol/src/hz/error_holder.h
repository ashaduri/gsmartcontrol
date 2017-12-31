/**************************************************************************
 Copyright:
      (C) 2008 - 2017  Alexander Shaduri <ashaduri 'at' gmail.com>
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
#include "error.h"
#include "debug.h"



namespace hz {



/// A class wishing to implement Error holding storage should inherit this.
/// Unless specified otherwise, all methods are thread-safe if the
/// thread-safe locking policy is provided.
/// \tparam LockPolicy_ A class with methods to lock / unlock current object access
/// for use in multi-threaded environments. See sync.h for more info.
class ErrorHolder {
	public:

		typedef std::vector<ErrorBase*> error_list_t;  ///< A list of ErrorBase* pointers
		typedef ptr_container<error_list_t> ptr_error_list_t;  ///< A list of auto-deleted pointers to error_list_t

		/// Virtual destructor
		virtual ~ErrorHolder() = default;


		/// Add an error to the error list
		template<class E>
		void push_error(const E& e)
		{
			E* copied = new E(e);  // clone e. it will be deleted when the list is destroyed.
			errors_.push_back(copied);

			error_warn(copied);
		}


		/// Remove last error from the error list
		template<class E>
		void pop_last_error(E& e)
		{
			e = static_cast<E>(errors_.pop_back());
		}


		/// Import errors from another object
		void import_errors(ErrorHolder& other)
		{
		    if (this == &other)
        		return;

			error_list_t tmp;  // non-pointer-basec container
			other.get_errors_cloned(tmp);
			errors_.insert(errors_.end(), tmp.begin(), tmp.end());  // copies the deletion ownership too
		}


		/// Check if there are any errors in this class.
		bool has_errors() const
		{
			return !errors_.empty();
		}


		/// Get a list of errors.
		error_list_t get_errors() const
		{
			// copy the pointers to deque, return it.
			error_list_t ret(errors_.begin(), errors_.end());
			return ret;
		}


		/// Get a cloned list of errors.
		/// You MUST delete the elements of the returned container!
		template<class ReturnedContainer>
		void get_errors_cloned(ReturnedContainer& put_here) const
		{
			errors_.clone_by_method_to(put_here);  // calls clone() on each element
		}


		// These functions are the recommended way to access the errors container.

		/// A begin() function for the error list.
		error_list_t::iterator errors_begin()
		{
			return errors_.begin();
		}

		/// A begin() function for the error list (const version).
		error_list_t::const_iterator errors_begin() const
		{
			return errors_.begin();
		}

		/// An end() function for the error list.
		error_list_t::iterator errors_end()
		{
			return errors_.end();
		}

		/// An end() function for the error list (const version).
		error_list_t::const_iterator errors_end() const
		{
			return errors_.end();
		}


		/// Clear the error list
		void clear_errors()
		{
			errors_.clear();
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

};




}  // ns




#endif

/// @}
