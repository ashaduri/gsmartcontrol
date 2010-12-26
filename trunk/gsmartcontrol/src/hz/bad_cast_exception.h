/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

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


struct bad_cast_except : virtual public std::exception {  // from <exception>

	bad_cast_except(const char* self_name = 0, const char* error_msg = 0)
		: src_type(typeid(void)), dest_type(typeid(void)),
		self_name_(self_name ? self_name : "bad_cast_except"),
		error_msg_(error_msg ? error_msg : "Type cast failed from \"%s\" to \"%s\".")
	{ }

	bad_cast_except(const std::type_info& src, const std::type_info& dest,
			const char* self_name = 0, const char* error_msg = 0)
		: src_type(src), dest_type(dest),
		self_name_(self_name ? self_name : "bad_cast_except"),
		error_msg_(error_msg ? error_msg : "Type cast failed from \"%s\" to \"%s\".")  // still need %s here for correct arg count for printf
	{ }

	virtual ~bad_cast_except() throw()
	{ }


	// note: we do quite a lot of construction here, but since it's not
	// an out-of-memory exception, what the heck.
	virtual const char* what() const throw()
	{
		std::string who = (self_name_.empty() ? "[unknown]" : self_name_);

		std::string from = (src_type == typeid(void) ? "[unknown]" : hz::type_name_demangle(src_type.name()));
		if (from.empty())
			from = src_type.name();

		std::string to = (dest_type == typeid(void) ? "[unknown]" : hz::type_name_demangle(dest_type.name()));
		if (to.empty())
			to = dest_type.name();

		return (msg_ = hz::string_sprintf((who + ": " + error_msg_).c_str(), from.c_str(), to.c_str())).c_str();
	}


	const std::type_info& src_type;
	const std::type_info& dest_type;

	private:
		mutable std::string msg_;  // This must be a member to avoid its destruction on function call return. use what().

		std::string self_name_;
		std::string error_msg_;
};


#else  // no RTTI variant:


struct bad_cast_except : virtual public std::exception {  // from <exception>

	bad_cast_except(const char* self_name = 0, const char* error_msg = 0)
		: self_name_(self_name ? self_name : "bad_cast_except"),
		error_msg_(error_msg ? error_msg : "Type cast failed.")
	{ }

	virtual ~bad_cast_except() throw()
	{ }

	virtual const char* what() const throw()
	{
		return (msg_ = (self_name_.empty() ? "[unknown]" : self_name_) + ": " + error_msg_).c_str();
	}

	private:
		mutable std::string msg_;  // This must be a member to avoid its destruction on function call return. use what().

		std::string self_name_;
		std::string error_msg_;
};


#endif



}  // ns




/*
// Example:
struct custom_cast_exception : public hz::bad_cast_except {

	custom_cast_exception() : hz::bad_cast_except("custom_cast_exception", "Cast failed from %s to %s.")
	{ }

#if !(defined DISABLE_RTTI && DISABLE_RTTI)
	custom_cast_exception(const std::type_info& src, const std::type_info& dest)
		: hz::bad_cast_except(src, dest, "custom_cast_exception", "Cast failed")
	{ }
#endif
};
*/


#if !(defined DISABLE_RTTI && DISABLE_RTTI)

#define DEFINE_BAD_CAST_EXCEPTION(name, rtti_msg, nortti_msg) \
	struct name : public hz::bad_cast_except { \
		name() : hz::bad_cast_except(#name, rtti_msg) \
		{ } \
		name(const std::type_info& src, const std::type_info& dest) \
			: hz::bad_cast_except(src, dest, #name, rtti_msg) \
		{ } \
	}

#else  // no rtti:

#define DEFINE_BAD_CAST_EXCEPTION(name, rtti_msg, nortti_msg) \
	struct name : public hz::bad_cast_except { \
		name() : hz::bad_cast_except(#name, nortti_msg) \
		{ } \
	}

#endif



#if !(defined DISABLE_RTTI && DISABLE_RTTI)

#define THROW_CUSTOM_BAD_CAST(name, from, to) \
	THROW_FATAL(name(from, to))

#else  // no rtti:

#define THROW_CUSTOM_BAD_CAST(name, from, to) \
	THROW_FATAL(name())

#endif






#endif
