/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef LIBDEBUG_DEXCEPT_H
#define LIBDEBUG_DEXCEPT_H

#include <cstring>  // strcpy / strlen
#include <exception>  // std::exception



// this is thrown on internal errors

struct debug_internal_error : virtual public std::exception {  // from <exception>

	debug_internal_error(const char* msg)
	{
		msg_ = std::strcpy(new char[std::strlen(msg) + 1], msg);
	}

	virtual ~debug_internal_error() throw()
	{
		delete[] msg_;
		msg_ = 0;  // protect from double-deletion compiler bugs
	}

	// Note: messages in exceptions are not newline-terminated.
 	virtual const char* what() const throw()
	{
		return msg_;
	}

	private:
		char* msg_;
};




// this is thrown on usage errors

struct debug_usage_error : virtual public std::exception {  // from <exception>

	debug_usage_error(const char* msg)
	{
		msg_ = std::strcpy(new char[std::strlen(msg) + 1], msg);
	}

	virtual ~debug_usage_error() throw()
	{
		delete[] msg_;
		msg_ = 0;  // protect from double-deletion compiler bugs
	}

	// Note: messages in exceptions are not newline-terminated.
 	virtual const char* what() const throw()
	{
		return msg_;
	}

	private:
		char* msg_;
};






#endif
