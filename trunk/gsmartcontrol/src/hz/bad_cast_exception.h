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

#ifndef HZ_BAD_CAST_EXCEPTION_H
#define HZ_BAD_CAST_EXCEPTION_H

#include "hz_config.h"  // feature macros

#include <exception>  // std::exception
#include <string>
#include <typeinfo>  // std::type_info

#include "system_specific.h"  // type_name_demangle
#include "string_sprintf.h"  // hz::string_sprintf



namespace hz {



/// Children of this class are thrown from various casting functions
class bad_cast_except : virtual public std::exception {  // from <exception>
	public:

		/// Constructor
		/// \param self_name child class name
		/// \param error_msg error message
		bad_cast_except(const char* self_name = 0, const char* error_msg = 0)
			: src_type(typeid(void)), dest_type(typeid(void)),
			self_name_(self_name ? self_name : "bad_cast_except"),
			error_msg_(error_msg ? error_msg : "Type cast failed from \"%s\" to \"%s\".")
		{ }

		/// Constructor
		bad_cast_except(const std::type_info& src, const std::type_info& dest,
				const char* self_name = 0, const char* error_msg = 0)
			: src_type(src), dest_type(dest),
			self_name_(self_name ? self_name : "bad_cast_except"),
			error_msg_(error_msg ? error_msg : "Type cast failed from \"%s\" to \"%s\".")  // still need %s here for correct arg count for printf
		{ }

		/// Virtual destructor
		virtual ~bad_cast_except() throw()
		{ }


		/// Reimplemented from std::exception
		virtual const char* what() const throw()
		{
			// Note: we do quite a lot of construction here, but since it's not
			// an out-of-memory exception, what the heck.
			std::string who = (self_name_.empty() ? "[unknown]" : self_name_);

			std::string from = (src_type == typeid(void) ? "[unknown]" : hz::type_name_demangle(src_type.name()));
			if (from.empty())
				from = src_type.name();

			std::string to = (dest_type == typeid(void) ? "[unknown]" : hz::type_name_demangle(dest_type.name()));
			if (to.empty())
				to = dest_type.name();

			return (msg_ = hz::string_sprintf((who + ": " + error_msg_).c_str(), from.c_str(), to.c_str())).c_str();
		}


		const std::type_info& src_type;  ///< Cast source type info
		const std::type_info& dest_type;  ///< Cast destination type info


	private:

		mutable std::string msg_;  ///< This must be a member to avoid its destruction on function call return. use what().

		std::string self_name_;  ///< The exception class name
		std::string error_msg_;  ///< Error message
};


}  // ns




/*
// Example:
class custom_cast_exception : public hz::bad_cast_except {
	public:

		custom_cast_exception() : hz::bad_cast_except("custom_cast_exception", "Cast failed from %s to %s.")
		{ }

		custom_cast_exception(const std::type_info& src, const std::type_info& dest)
			: hz::bad_cast_except(src, dest, "custom_cast_exception", "Cast failed")
		{ }
};
*/


/// \def DEFINE_BAD_CAST_EXCEPTION(name, rtti_msg, nortti_msg)
/// Define an exception class named \c name that is a child of hz::bad_cast_except,
/// and shows \c rtti_msg if RTTI is enabled and nortti_msg if RTTI is disabled.


#define DEFINE_BAD_CAST_EXCEPTION(name, rtti_msg, nortti_msg) \
	class name : public hz::bad_cast_except { \
		public: \
			name() : hz::bad_cast_except(#name, rtti_msg) \
			{ } \
			name(const std::type_info& src, const std::type_info& dest) \
				: hz::bad_cast_except(src, dest, #name, rtti_msg) \
			{ } \
	}






#endif

/// @}
