/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup libdebug
/// \weakgroup libdebug
/// @{

#ifndef LIBDEBUG_DEXCEPT_H
#define LIBDEBUG_DEXCEPT_H

#include <cstddef>  // std::size_t
#include <cstring>  // std::strncpy / std::strlen
#include <exception>  // std::exception



/// Exception thrown on internal libdebug errors
struct debug_internal_error : virtual public std::exception {

	/// Constructor
	debug_internal_error(const char* msg)
	{
		std::size_t buf_len = std::strlen(msg) + 1;
		msg_ = std::strncpy(new char[buf_len], msg, buf_len);
	}

	/// Virtual destructor
	virtual ~debug_internal_error()
	{
		delete[] msg_;
	}

	/// Reimplemented. Note: messages in exceptions are not newline-terminated.
 	const char* what() const noexcept override
	{
		return msg_;
	}

	private:
		char* msg_;  ///< The error message
};




/// Exception thrown on libdebug API usage errors
struct debug_usage_error : virtual public std::exception {  // from <exception>

	/// Constructor
	debug_usage_error(const char* msg)
	{
		std::size_t buf_len = std::strlen(msg) + 1;
		msg_ = std::strncpy(new char[buf_len], msg, buf_len);
	}

	/// Virtual destructor
	virtual ~debug_usage_error()
	{
		delete[] msg_;
	}

	/// Reimplemented. Note: messages in exceptions are not newline-terminated.
 	const char* what() const noexcept override
	{
		return msg_;
	}

	private:
		char* msg_;  ///< The error message
};






#endif

/// @}
