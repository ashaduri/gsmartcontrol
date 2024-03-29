/******************************************************************************
License: Zlib
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup libdebug
/// \weakgroup libdebug
/// @{

#ifndef LIBDEBUG_DOUT_H
#define LIBDEBUG_DOUT_H

#include <string>
// Note: Sun compiler refuses to compile without <ostream> (iosfwd is not enough).
// Since every useful operator << is defined in ostream, we include it here anyway.
#include <ostream>  // std::ostream
#include <utility>

#include "hz/system_specific.h"  // HZ_FUNC_PRINTF_ISO_CHECK

#include "dflags.h"



/// Get a libdebug-handled stream for \c level and \c domain.
/// \throw debug_usage_error if invalid domain or level.
std::ostream& debug_out(debug_level::flag level, const std::string& domain);



// These are macros to be able to easily compile-out per-level output.

/// Send an output to debug stream. For example:
/// \code
/// debug_out_error("app", DBG_FUNC_MSG << "Error in structure consistency.\n");
/// debug_out_dump("app", "Error value: " << value << ".\n");
/// \endcode
#define debug_out_dump(domain, output) \
	debug_out(debug_level::dump, domain) << output

/// Send an output to debug stream. \see debug_out_dump().
#define debug_out_info(domain, output) \
	debug_out(debug_level::info, domain) << output

/// Send an output to debug stream. \see debug_out_dump().
#define debug_out_warn(domain, output) \
	debug_out(debug_level::warn, domain) << output

/// Send an output to debug stream. \see debug_out_dump().
#define debug_out_error(domain, output) \
	debug_out(debug_level::error, domain) << output

/// Send an output to debug stream. \see debug_out_dump().
#define debug_out_fatal(domain, output) \
	debug_out(debug_level::fatal, domain) << output



/// Start prefix printing. Useful for large dumps where you don't want prefixes to
/// be printed on each debug_* call.
void debug_begin();

/// Stop prefix printing.
void debug_end();




// Source position tracking


namespace debug_internal {

	/// Source position object for sending to libdebug streams, prints source position.
	struct DebugSourcePos {

		/// Constructor
		inline DebugSourcePos(std::string par_file, int line_, std::string par_func_name, std::string par_func)
				: func_name(std::move(par_func_name)), func(std::move(par_func)), line(line_), file(std::move(par_file))
		{
			enabled_types.set(debug_pos::func_name);
			enabled_types.set(debug_pos::line);
			enabled_types.set(debug_pos::file);
		}

		/// Formatted output string
		[[nodiscard]] std::string str() const;


		std::string func_name;  ///< Function name only
		std::string func;  ///< Function name with namespaces and classes
		int line = 0;  ///< Source line
		std::string file;  ///< Source file

		debug_pos::flags enabled_types;  ///< Enabled formatting types
	};


	/// Output operator
	inline std::ostream& operator<< (std::ostream& os, const DebugSourcePos& pos)
	{
		return os << pos.str();
	}


	/// Format the function name
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

/// Current file as const char*.
#define DBG_FILE __FILE__

/// Current line as integer constant.
#define DBG_LINE __LINE__


/// \def DBG_FUNC_NAME
/// Function name (without classes / namespaces) only, e.g. "main", as const char*.
#define DBG_FUNC_NAME __func__


/// \def DBG_FUNC_PRNAME
/// Function pretty name is the whole function prototype,
/// including return type and classes / namespaces, as const char*.
#ifdef __GNUC__
	#define DBG_FUNC_PRNAME __PRETTY_FUNCTION__
#else
	#define DBG_FUNC_PRNAME DBG_FUNC_NAME
#endif


/// "class::function()", as const char*.
/// static_cast is needed to avoid clang-tidy errors about converting array to pointer.
#define DBG_FUNC (debug_internal::format_function_msg(static_cast<const char*>(DBG_FUNC_PRNAME), false).c_str())

/// "class::function(): ", as const char*.
/// static_cast is needed to avoid clang-tidy errors about converting array to pointer.
#define DBG_FUNC_MSG (debug_internal::format_function_msg(static_cast<const char*>(DBG_FUNC_PRNAME), true).c_str())


/// When sent into std::ostream, this object prints current source position.
#define DBG_POS debug_internal::DebugSourcePos(DBG_FILE, DBG_LINE, static_cast<const char*>(DBG_FUNC_NAME), DBG_FUNC)


/// A standalone function-like macro, prints "Trace point "a" reached at (source position)"
/// (\c a is the macro parameter).
#define DBG_TRACE_POINT_MSG(a) debug_out_dump("default", "TRACE point \"" << #a << "\" reached at " << DBG_POS << ".\n")

/// A standalone function-like macro, prints "Trace point reached at (source position)".
#define DBG_TRACE_POINT_AUTO debug_out_dump("default", "TRACE point reached at " << DBG_POS << ".\n")


/// A standalone function-like macro, prints "ENTER: "function_name"".
#define DBG_FUNCTION_ENTER_MSG debug_out_dump("default", "ENTER: \"" << DBG_FUNC << "\"\n")

/// A standalone function-like macro, prints "EXIT:  "function_name"".
#define DBG_FUNCTION_EXIT_MSG debug_out_dump("default", "EXIT:  \"" << DBG_FUNC << "\"\n")


/// A standalone function-like macro, prints \c msg if \c cond evaluates to false.
#define DBG_ASSERT_MSG(cond, msg) \
	if (true) { \
		if (!(cond)) \
			debug_out_error("default", (msg) << "\n"); \
	} else (void)0


/// A standalone function-like macro, prints generic message if \c cond evaluates to false.
#define DBG_ASSERT(cond) \
	if (true) { \
		if (!(cond)) \
			debug_out_error("default", "ASSERTION FAILED: " << #cond << " at " << DBG_POS << "\n"); \
	} else (void)0


/// Prints generic message to error-level if assertion fails. Don't need to send into stream, it prints by itself.
/// Returns from the function on assertion failure.
#define DBG_ASSERT_RETURN(cond, return_value) \
	do { \
		if (!(cond)) { \
			debug_out_error("default", "ASSERTION FAILED: " << #cond << " at " << DBG_POS << "\n"); \
			return (return_value); \
		} \
	} while(false)


/// Prints generic message to error-level if assertion fails. Don't need to send into stream, it prints by itself.
/// Returns from the function on assertion failure.
#define DBG_ASSERT_RETURN_NONE(cond) \
	do { \
		if (!(cond)) { \
			debug_out_error("default", "ASSERTION FAILED: " << #cond << " at " << DBG_POS << "\n"); \
			return; \
		} \
	} while(false)




// ------------------ Indentation and manipulators



/// Increase indentation level for all debug levels
void debug_indent_inc(int by = 1);

/// Decrease indentation level for all debug levels
void debug_indent_dec(int by = 1);

/// Reset indentation level to 0 for all debug levels
void debug_indent_reset();





namespace debug_internal {

	struct DebugIndent;
	struct DebugUnindent;
	struct DebugResetIndent;


	/// A stream manipulator that increases the indentation level
	struct DebugIndent {
		/// Constructor
		constexpr explicit DebugIndent(int indent_level = 1) : by(indent_level)
		{ }

		/// Constructs a new DebugIndent object
		DebugIndent operator() (int indent_level = 1)
		{
			return DebugIndent(indent_level);
		}

		int by = 1;  ///< Number of indentation levels to increase with (may be negative)
	};


	/// A stream manipulator that decreases the indentation level
	struct DebugUnindent {
		/// Constructor
		constexpr explicit DebugUnindent(int unindent_level = 1) : by(unindent_level)
		{ }

		/// Constructs a new DebugUnindent object
		DebugUnindent operator() (int unindent_level = 1)
		{
			return DebugUnindent(unindent_level);
		}

		int by = 1;  ///< Number of indentation levels to decrease with (may be negative)
	};


	/// A stream manipulator that resets the indentation level to 0
	struct DebugResetIndent {

		/// Constructs a new DebugResetIndent object
		DebugResetIndent operator() ()  // just for consistency
		{
			return *this;
		}
	};



	// These operators need to be inside the same namespace that their
	// operands are for ADL to work inside _other_ namespaces.

	/// A stream manipulator operator
	inline std::ostream& operator<< (std::ostream& os, const debug_internal::DebugIndent& m)
	{
		debug_indent_inc(m.by);
		return os;
	}


	/// A stream manipulator operator
	inline std::ostream& operator<< (std::ostream& os, const debug_internal::DebugUnindent& m)
	{
		debug_indent_dec(m.by);
		return os;
	}


	/// A stream manipulator operator
	inline std::ostream& operator<< (std::ostream& os,  [[maybe_unused]] const debug_internal::DebugResetIndent& m)
	{
		debug_indent_reset();
		return os;
	}


}  // ns



/// Manupulator object - send this to libdebug-backed stream to increase the indentation level by 1.
constexpr inline debug_internal::DebugIndent debug_indent;

/// Manupulator object - send this to libdebug-backed stream to decrease the indentation level by 1.
constexpr inline debug_internal::DebugUnindent debug_unindent;

/// Manupulator object - send this to libdebug-backed stream to reset the indentation level to 0.
constexpr inline debug_internal::DebugResetIndent debug_resindent;






#endif

/// @}
