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

#ifndef HZ_ERROR_HOLDER_H
#define HZ_ERROR_HOLDER_H

#include <vector>
#include <memory>

#include "error.h"
#include "debug.h"



namespace hz {



/// A class wishing to implement Error holding storage should inherit this.
class ErrorHolder {
	public:

		using error_list_t = std::vector<std::shared_ptr<ErrorBase>>;  ///< A list of ErrorBase* pointers

		/// Virtual destructor
		virtual ~ErrorHolder() = default;


		/// Add an error to the error list
		template<class E>
		void push_error(const E& e)
		{
			auto cloned = std::make_shared<E>(e);
			errors_.push_back(cloned);
			error_warn(cloned.get());
		}


		/// Check if there are any errors in this class.
		bool has_errors() const
		{
			return !errors_.empty();
		}


		/// Get a list of errors.
		error_list_t get_errors() const
		{
			return errors_;
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
			ErrorLevel level = e->get_level();

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

		error_list_t errors_;  ///< Error list. The newest errors at the end.

};




}  // ns




#endif

/// @}
