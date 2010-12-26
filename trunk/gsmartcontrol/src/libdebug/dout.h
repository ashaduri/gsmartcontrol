/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef LIBDEBUG_DOUT_H
#define LIBDEBUG_DOUT_H

#include <string>
// Note: Sun compiler refuses to compile without <ostream> (iosfwd is not enough).
// Since every useful operator << is defined in ostream, we include it here anyway.
#include <ostream>  // std::ostream

#include "hz/system_specific.h"
#include "hz/hz_config.h"  // HAVE_CXX__FUNC

#include "dflags.h"



// These functions are thread-safe if the state doesn't change. That is, you may use
// the output functions in different threads, but if you modify the state (channels,
// default format flags, etc...), then you _must_ stop all libdebug activity in _all_
// threads except the one you're modifying with.




// this may throw for invalid domain or level.
std::ostream& debug_out(debug_level::flag level, const std::string& domain);



#define debug_out_dump(domain, output) \
	debug_out(debug_level::dump, domain) << output

#define debug_out_info(domain, output) \
	debug_out(debug_level::info, domain) << output

#define debug_out_warn(domain, output) \
	debug_out(debug_level::warn, domain) << output

#define debug_out_error(domain, output) \
	debug_out(debug_level::error, domain) << output

#define debug_out_fatal(domain, output) \
	debug_out(debug_level::fatal, domain) << output




void debug_print(debug_level::flag level, const std::string& domain,
		const char* format, ...) HZ_FUNC_PRINTF_CHECK(3, 4);



#define debug_print_dump(domain, ...) \
	debug_print(debug_level::dump, domain, __VA_ARGS__)

#define debug_print_info(domain, ...) \
	debug_print(debug_level::info, domain, __VA_ARGS__)

#define debug_print_warn(domain, ...) \
	debug_print(debug_level::warn, domain, __VA_ARGS__)

#define debug_print_error(domain, ...) \
	debug_print(debug_level::error, domain, __VA_ARGS__)

#define debug_print_fatal(domain, ...) \
	debug_print(debug_level::fatal, domain, __VA_ARGS__)





// Start / stop prefix printing. Useful for large dumps

void debug_begin();

void debug_end();




// Source position tracking


namespace debug_internal {


	struct DebugSourcePos {

		inline DebugSourcePos(const std::string& file_, unsigned int line_, const std::string& func_name_, const std::string& func_)
				: func_name(func_name_), func(func_), line(line_), file(file_), enabled_types(debug_pos::def)
		{ }


		std::string str() const;


		std::string func_name;  // name only
		std::string func;  // name with namespaces and classes
		unsigned int line;
		std::string file;

		debug_pos::type enabled_types;
	};


	// it can serve as a manipulator
	inline std::ostream& operator<< (std::ostream& os, const DebugSourcePos& pos)
	{
		return os << pos.str();
	}


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




// __BASE_FILE__, __FILE__, __LINE__, __func__, __FUNCTION__, __PRETTY_FUNCTION__

// These two may seem pointless, but they actually help to implement
// zero-overhead (if you define them to something else when libdebug is disabled).

// Current file as const char*.
#define DBG_FILE __FILE__

// Current line as unsigned int.
#define DBG_LINE __LINE__


// Function name (without classes / namespaces) only, e.g. "main".
#ifdef HAVE_CXX__FUNC
	#define DBG_FUNC_NAME __func__
#else
	#define DBG_FUNC_NAME __FUNCTION__
#endif

// Pretty name is the whole prototype, including return type and classes / namespaces.
#ifdef __GNUC__
	#define DBG_FUNC_PRNAME __PRETTY_FUNCTION__
#else
	#define DBG_FUNC_PRNAME DBG_FUNC_NAME
#endif


// "class::function()"
#define DBG_FUNC (debug_internal::format_function_msg(DBG_FUNC_PRNAME, false).c_str())

// "class::function(): "
#define DBG_FUNC_MSG (debug_internal::format_function_msg(DBG_FUNC_PRNAME, true).c_str())


// An object to send into a stream. Prints position.
#define DBG_POS debug_internal::DebugSourcePos(DBG_FILE, DBG_LINE, DBG_FUNC_NAME, DBG_FUNC)


// Prints "Trace point a reached". Don't need to send into stream, it prints by itself.
#define DBG_TRACE_POINT_MSG(a) debug_out_dump("default", "TRACE point \"" << #a << "\" reached at " << DBG_POS << ".\n")

// Prints "Trace point reached". Don't need to send into stream, it prints by itself.
#define DBG_TRACE_POINT_AUTO debug_out_dump("default", "TRACE point reached at " << DBG_POS << ".\n")


// Prints "ENTER: function_name". Don't need to send into stream, it prints by itself.
#define DBG_FUNCTION_ENTER_MSG debug_out_dump("default", "ENTER: \"" << DBG_FUNC << "\"\n")

// Prints "EXIT:  function_name". Don't need to send into stream, it prints by itself.
#define DBG_FUNCTION_EXIT_MSG debug_out_dump("default", "EXIT:  \"" << DBG_FUNC << "\"\n")


// Prints msg if assertion fails. Don't need to send into stream, it prints by itself.
#define DBG_ASSERT_MSG(cond, msg) \
	if (true) { \
		if (!(cond)) \
			debug_out_error("default", (msg) << "\n"); \
	} else (void)0


// Prints generic message if assertion fails. Don't need to send into stream, it prints by itself.
#define DBG_ASSERT(cond) \
	if (true) { \
		if (!(cond)) \
			debug_out_error("default", "ASSERTION FAILED: " << #cond << " at " << DBG_POS << "\n"); \
	} else (void)0




// ------------------ Indentation and manipulators



// increase indentation level for all debug levels
void debug_indent_inc(int by = 1);

// decrease
void debug_indent_dec(int by = 1);

// reset
void debug_indent_reset();





namespace debug_internal {

	class DebugIndent;
	class DebugUnindent;
	class DebugResetIndent;



	struct DebugIndent {
		DebugIndent(int indent_level = 1) : by(indent_level)
		{ }

		DebugIndent operator() (int indent_level = 1)
		{
			return DebugIndent(indent_level);
		}

		int by;
	};


	struct DebugUnindent {
		DebugUnindent(int unindent_level = 1) : by(unindent_level)
		{ }

		DebugUnindent operator() (int unindent_level = 1)
		{
			return DebugUnindent(unindent_level);
		}

		int by;
	};


	struct DebugResetIndent {
		DebugResetIndent operator() ()  // just for consistency
		{
			return *this;
		}
	};



	// These operators need to be inside the same namespace that their
	// operands are for Koenig lookup to work inside _other_ namespaces.

	inline std::ostream& operator<< (std::ostream& os, debug_internal::DebugIndent& m)
	{
		debug_indent_inc(m.by);
		return os;
	}


	inline std::ostream& operator<< (std::ostream& os, debug_internal::DebugUnindent& m)
	{
		debug_indent_dec(m.by);
		return os;
	}


	inline std::ostream& operator<< (std::ostream& os, debug_internal::DebugResetIndent& m)
	{
		debug_indent_reset();
		return os;
	}



	// manupulator objects
	extern DebugIndent debug_indent;
	extern DebugUnindent debug_unindent;
	extern DebugResetIndent debug_resindent;


}  // ns



// manupulator objects
using debug_internal::debug_indent;
using debug_internal::debug_unindent;
using debug_internal::debug_resindent;









#endif
