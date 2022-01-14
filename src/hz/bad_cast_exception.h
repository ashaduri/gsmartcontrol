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

#ifndef HZ_BAD_CAST_EXCEPTION_H
#define HZ_BAD_CAST_EXCEPTION_H

#include <exception>  // std::exception
#include <string>
#include <typeinfo>  // std::type_info

#include "system_specific.h"  // type_name_demangle
#include "string_sprintf.h"  // hz::string_sprintf



namespace hz {



/// Children of this class are thrown from various casting functions
class bad_cast_except : public std::exception {  // from <exception>
	public:

		/// Constructor
		/// \param src source type
		/// \param dest destination type
		/// \param self_name child class name
		/// \param error_msg error message
		bad_cast_except(const std::type_info& src, const std::type_info& dest,
				const char* self_name = nullptr, const char* error_msg = nullptr)
			: src_type_(src), dest_type_(dest),
			self_name_(self_name ? self_name : "bad_cast_except"),
			error_msg_(error_msg ? error_msg : "Type cast failed from \"%s\" to \"%s\".")  // still need %s here for correct arg count for printf
		{ }


		/// Reimplemented from std::exception
		const char* what() const noexcept override
		{
			// Note: we do quite a lot of construction here, but since it's not
			// an out-of-memory exception, what the heck.
			std::string who = (self_name_.empty() ? "[unknown]" : self_name_);

			std::string from = (src_type_ == typeid(void) ? "[unknown]" : hz::type_name_demangle(src_type_.name()));
			if (from.empty())
				from = src_type_.name();

			std::string to = (dest_type_ == typeid(void) ? "[unknown]" : hz::type_name_demangle(dest_type_.name()));
			if (to.empty())
				to = dest_type_.name();

			return (msg_ = hz::string_sprintf((who + ": " + error_msg_).c_str(), from.c_str(), to.c_str())).c_str();
		}


		/// Get source type
		const std::type_info& src_type() const
		{
			return src_type_;
		}


		/// Get destination type
		const std::type_info& dest_type() const
		{
			return dest_type_;
		}


	private:

		const std::type_info& src_type_;  ///< Cast source type info. Can be a reference since type_info objects are guaranteed to live forever.
		const std::type_info& dest_type_;  ///< Cast destination type info

		mutable std::string msg_;  ///< This must be a member to avoid its destruction on function call return. use what().

		std::string self_name_;  ///< The exception class name
		std::string error_msg_;  ///< Error message
};


}  // ns






#endif

/// @}
