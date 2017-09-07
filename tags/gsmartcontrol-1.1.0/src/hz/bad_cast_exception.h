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

#ifndef HZ_BAD_CAST_EXCEPTION_H
#define HZ_BAD_CAST_EXCEPTION_H

#include "hz_config.h"  // feature macros

// we need these for instantiation, even if exceptions are disabled.
#include <exception>  // std::exception
#include <string>

#if !(defined DISABLE_RTTI && DISABLE_RTTI)
	#include <typeinfo>  // std::type_info
#endif

#include "exceptions.h"  // THROW_FATAL
#include "system_specific.h"  // type_name_demangle
#include "string_sprintf.h"  // hz::string_sprintf



namespace hz {



#if !(defined DISABLE_RTTI && DISABLE_RTTI)


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


#else  // no RTTI variant:


	/// Children of this class are thrown from various casting functions
class bad_cast_except : virtual public std::exception {  // from <exception>
	public:

		/// Constructor
		/// \param self_name child class name
		/// \param error_msg error message
		bad_cast_except(const char* self_name = 0, const char* error_msg = 0)
			: self_name_(self_name ? self_name : "bad_cast_except"),
			error_msg_(error_msg ? error_msg : "Type cast failed.")
		{ }

		/// Virtual destructor
		virtual ~bad_cast_except() throw()
		{ }

		/// Reimplemented from std::exception
		virtual const char* what() const throw()
		{
			return (msg_ = (self_name_.empty() ? "[unknown]" : self_name_) + ": " + error_msg_).c_str();
		}


	private:

		mutable std::string msg_;  ///< This must be a member to avoid its destruction on function call return. use what().

		std::string self_name_;  ///< The exception class name
		std::string error_msg_;  ///< Error message
};


#endif



}  // ns




/*
// Example:
class custom_cast_exception : public hz::bad_cast_except {
	public:

		custom_cast_exception() : hz::bad_cast_except("custom_cast_exception", "Cast failed from %s to %s.")
		{ }

#if !(defined DISABLE_RTTI && DISABLE_RTTI)
		custom_cast_exception(const std::type_info& src, const std::type_info& dest)
			: hz::bad_cast_except(src, dest, "custom_cast_exception", "Cast failed")
		{ }
#endif
};
*/


/// \def DEFINE_BAD_CAST_EXCEPTION(name, rtti_msg, nortti_msg)
/// Define an exception class named \c name that is a child of hz::bad_cast_except,
/// and shows \c rtti_msg if RTTI is enabled and nortti_msg if RTTI is disabled.


#if !(defined DISABLE_RTTI && DISABLE_RTTI)

#define DEFINE_BAD_CAST_EXCEPTION(name, rtti_msg, nortti_msg) \
	class name : public hz::bad_cast_except { \
		public: \
			name() : hz::bad_cast_except(#name, rtti_msg) \
			{ } \
			name(const std::type_info& src, const std::type_info& dest) \
				: hz::bad_cast_except(src, dest, #name, rtti_msg) \
			{ } \
	}

#else  // no rtti:

#define DEFINE_BAD_CAST_EXCEPTION(name, rtti_msg, nortti_msg) \
	class name : public hz::bad_cast_except { \
		public: \
			name() : hz::bad_cast_except(#name, nortti_msg) \
			{ } \
	}

#endif



/// \def THROW_CUSTOM_BAD_CAST(name, from, to)
/// Throw a bad cast exception of class \c name, with \c from
/// and \c to being std::type_info class objects.

#if !(defined DISABLE_RTTI && DISABLE_RTTI)

#define THROW_CUSTOM_BAD_CAST(name, from, to) \
	THROW_FATAL(name(from, to))

#else  // no rtti:

#define THROW_CUSTOM_BAD_CAST(name, from, to) \
	THROW_FATAL(name())

#endif






#endif

/// @}
