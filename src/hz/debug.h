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

#ifndef HZ_DEBUG_H
#define HZ_DEBUG_H

#include <cstdio>  // std::fprintf(), std::vfprintf()


/*
#include <cassert>
#ifndef ASSERT  // assert() is undefined if NDEBUG is defined.
#	define ASSERT(a) assert(a)
#endif
*/

/**
\file
This file is a link between libhz (and its users) and libdebug.
It provides a way to write in libdebug-like API without actually
using libdebug.

Note that it provides only output functions. The setup functions
cannot be emulated (but you probably won't need them in libraries
anyway).
*/


// Use libdebug as is
#if defined(HZ_USE_LIBDEBUG) && (HZ_USE_LIBDEBUG)

	// only output functions
	#include "libdebug/libdebug_mini.h"


#else

	// undef them in case libdebug was included
	#ifdef debug_out_dump
		#undef debug_out_dump
	#endif
	#ifdef debug_out_info
		#undef debug_out_info
	#endif
	#ifdef debug_out_warn
		#undef debug_out_warn
	#endif
	#ifdef debug_out_error
		#undef debug_out_error
	#endif
	#ifdef debug_out_fatal
		#undef debug_out_fatal
	#endif

	#ifdef DBG_FILE
		#undef DBG_FILE
	#endif
	#ifdef DBG_LINE
		#undef DBG_LINE
	#endif
	#ifdef DBG_FUNC_NAME
		#undef DBG_FUNC_NAME
	#endif
	#ifdef DBG_FUNC_PRNAME
		#undef DBG_FUNC_PRNAME
	#endif
	#ifdef DBG_FUNC
		#undef DBG_FUNC
	#endif
	#ifdef DBG_FUNC_MSG
		#undef DBG_FUNC_MSG
	#endif

	#ifdef DBG_POS
		#undef DBG_POS
	#endif

	#ifdef DBG_TRACE_POINT_MSG
		#undef DBG_TRACE_POINT_MSG
	#endif
	#ifdef DBG_TRACE_POINT_AUTO
		#undef DBG_TRACE_POINT_AUTO
	#endif

	#ifdef DBG_FUNCTION_ENTER_MSG
		#undef DBG_FUNCTION_ENTER_MSG
	#endif
	#ifdef DBG_FUNCTION_EXIT_MSG
		#undef DBG_FUNCTION_EXIT_MSG
	#endif

	#ifdef DBG_ASSERT_MSG
		#undef DBG_ASSERT_MSG
	#endif
	#ifdef DBG_ASSERT
		#undef DBG_ASSERT
	#endif


	// emulate libdebug API through std::cerr
	#if defined(HZ_EMULATE_LIBDEBUG) && HZ_EMULATE_LIBDEBUG

		#include <iostream>
		#include <cstdio>
		#include <string>

		// cerr / stderr have no buffering, so no need to sync them with each other.


		#define debug_out_dump(domain, output) \
			std::cerr << "<dump>  [" << (domain) << "] " << output

		#define debug_out_info(domain, output) \
			std::cerr << "<info>  [" << (domain) << "] " << output

		#define debug_out_warn(domain, output) \
			std::cerr << "<warn>  [" << (domain) << "] " << output

		#define debug_out_error(domain, output) \
			std::cerr << "<error> [" << (domain) << "] " << output

		#define debug_out_fatal(domain, output) \
			std::cerr << "<fatal> [" << (domain) << "] " << output


		#define DBG_FILE __FILE__
		#define DBG_LINE __LINE__
		#define DBG_FUNC_NAME __func__

		#ifdef __GNUC__
			#define DBG_FUNC_PRNAME __PRETTY_FUNCTION__
		#else
			#define DBG_FUNC_PRNAME DBG_FUNC_NAME
		#endif


		#include <string>

		namespace hz {
			namespace internal {
				inline std::string format_function_msg(const std::string& func, bool add_suffix)
				{
					// if it's "bool<unnamed>::A::func(int)" or "bool test::A::func(int)",
					// remove the return type and parameters.
					std::string::size_type endpos = func.find('(');
					if (endpos == std::string::npos)
						endpos = func.size();

					// search for first space (after the parameter), or "<unnamed>".
					std::string::size_type pos = func.find_first_of(" >");
					if (pos != std::string::npos) {
						if (func[pos] == '>')
							pos += 2;  // skip ::
						++pos;  // skip whatever character we're over
						// debug_out_info("default", "pos: " << pos << ", endpos: " << endpos << "\n");
						return func.substr(pos >= endpos ? 0 : pos, endpos - pos) + (add_suffix ? "(): " : "()");
					}
					return func.substr(0, endpos) + (add_suffix ? "(): " : "()");
				}
			}
		}

		#define DBG_FUNC (hz::internal::format_function_msg(DBG_FUNC_PRNAME, false).c_str())

		#define DBG_FUNC_MSG (hz::internal::format_function_msg(DBG_FUNC_PRNAME, true).c_str())


		// Note: DBG_POS is not an object if emulated or disabled! Only valid for outputting to streams.
		#define DBG_POS "(function: " << DBG_FUNC_NAME << "(), file: " << DBG_FILE \
				<< ", line: " << DBG_LINE << ")"


		#define DBG_TRACE_POINT_MSG(a) debug_out_dump("default", "Trace point \"" << #a << "\" reached at " << DBG_POS << ".\n")
		#define DBG_TRACE_POINT_AUTO debug_out_dump("default", "Trace point reached at " << DBG_POS << ".\n")

		#define DBG_FUNCTION_ENTER_MSG debug_out_dump("default", "ENTER: \"" << DBG_FUNC << "\"\n")
		#define DBG_FUNCTION_EXIT_MSG debug_out_dump("default", "EXIT:  \"" << DBG_FUNC << "\"\n")


		#define DBG_ASSERT_MSG(cond, msg) \
		if (true) { \
			if (!(cond)) \
				debug_out_error("default", (msg) << "\n"); \
		} else (void)0

		#define DBG_ASSERT(cond) \
		if (true) { \
			if (!(cond)) \
				debug_out_error("default", "ASSERTION FAILED: " << #cond << " at " << DBG_POS << "\n"); \
		} else (void)0


	// No output at all
	#else

		// do/while block is needed to:
		// 1. make " if(a) debug_out_info(); f(); " work correctly.
		// 2. require terminating semicolon.
		#define debug_out_dump(domain, output) if(true){}else(void)0
		#define debug_out_info(domain, output) if(true){}else(void)0
		#define debug_out_warn(domain, output) if(true){}else(void)0
		#define debug_out_error(domain, output) if(true){}else(void)0
		#define debug_out_fatal(domain, output) if(true){}else(void)0

		#define DBG_FILE ""
		#define DBG_LINE 0

		#define DBG_FUNC_NAME ""
		#define DBG_FUNC_PRNAME ""
		#define DBG_FUNC ""
		#define DBG_FUNC_MSG ""

		// Note: DBG_POS is not an object if emulated or disabled!
		#define DBG_POS ""

		#define DBG_TRACE_POINT_MSG(a) if(true){}else(void)0
		#define DBG_TRACE_POINT_AUTO if(true){}else(void)0

		#define DBG_FUNCTION_ENTER_MSG if(true){}else(void)0
		#define DBG_FUNCTION_EXIT_MSG if(true){}else(void)0

		#define DBG_ASSERT_MSG(cond, msg) if(true){}else(void)0
		#define DBG_ASSERT(cond) if(true){}else(void)0

	#endif


	// other stuff, emulated or not

// 	#ifdef debug_begin
// 		#undef debug_begin
// 	#endif
	#define debug_begin() if(true){}else(void)0

// 	#ifdef debug_end
// 		#undef debug_end
// 	#endif
	#define debug_end() if(true){}else(void)0


	#define debug_indent_inc(...) if(true){}else(void)0
	#define debug_indent_dec(...) if(true){}else(void)0
	#define debug_indent_reset() if(true){}else(void)0

	#define debug_indent ""
	#define debug_unindent ""
	#define debug_resindent ""



#endif











#endif

/// @}
