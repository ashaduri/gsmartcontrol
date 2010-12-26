/**************************************************************************
 Copyright:
      (C) 2008 - 2009 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#include <string>
#include <iosfwd>  // std::ostream definition
#include <sstream>
#include <cstdarg>  // std::va_start, va_list macro and friends

#include "hz/tls.h"
#include "hz/string_sprintf.h"  // string_vsprintf()
#include "hz/exceptions.h"  // THROW_FATAL

#include "dout.h"
#include "dflags.h"
#include "dstate.h"
#include "dexcept.h"




// This may throw for invalid domain or level.
std::ostream& debug_out(debug_level::flag level, const std::string& domain)
{
	debug_internal::DebugState::domain_map_t& dm = debug_internal::get_debug_state().get_domain_map();

	debug_internal::DebugState::domain_map_t::iterator level_map = dm.find(domain);
	if (level_map == dm.end()) {  // no such domain
		std::string msg = "debug_out(): Debug state doesn't contain the requested domain: \"" + domain + "\".";

		if (domain != "default") {
			debug_out(debug_level::warn, "default") << msg << "\n";
			debug_out(debug_level::info, "default") << "Auto-creating the missing domain.\n";
			debug_register_domain(domain);
			debug_out(debug_level::warn, "default") << "The message follows:\n";
			return debug_out(level, domain);  // try again
		}

		// this is an internal error
		THROW_FATAL(debug_internal_error(msg.c_str()));
	}

	debug_internal::DebugState::level_map_t::iterator os = level_map->second.find(level);
	if (level_map == dm.end()) {
		std::string msg = std::string("debug_out(): Debug state doesn't contain the requested level ") +
				debug_level::get_name(level) + " in domain: \"" + domain + "\".";

		// this is an internal error
		THROW_FATAL(debug_internal_error(msg.c_str()));
	}

	return *(os->second);
}





void debug_print(debug_level::flag level, const std::string& domain, const char* format, ...)
{
	std::va_list ap;
	va_start(ap, format);

	std::string s = hz::string_vsprintf(format, ap);

	va_end(ap);
	debug_out(level, domain) << s;
}





// Start / stop prefix printing. Useful for large dumps

void debug_begin()
{
	// inside_begin is thread-local, no need to lock anything
	debug_internal::get_debug_state().push_inside_begin();
}


void debug_end()
{
	// inside_begin is thread-local, no need to lock anything
	debug_internal::get_debug_state().pop_inside_begin();
	// this is needed because else the contents won't be written until next write.
	debug_internal::get_debug_state().force_output();  // this call is thread-safe in read-only context.
}





namespace debug_internal {


	std::string DebugSourcePos::str() const
	{
		std::stringstream os;
		os << "(";

		if (enabled_types.to_ulong() & debug_pos::func_name) {
			os << "function: " << func_name;

		} else if (enabled_types.to_ulong() & debug_pos::func) {
			os << "function: " << func << "()";
		}

		if (enabled_types.to_ulong() & debug_pos::file) {
			if (os.str() != "(")
				os << ", ";
			os << "file: " << file;
		}

		if (enabled_types.to_ulong() & debug_pos::line) {
			if (os.str() != "(")
				os << ", ";
			os << "line: " << line;
		}

		os << ")";

		return os.str();
	}


}



// ------------------ Indentation and manipulators


// increase indentation level for all debug levels
void debug_indent_inc(int by)
{
	// indent level is thread-local, no need to lock anything
	int curr = debug_internal::get_debug_state().get_indent_level();
	debug_internal::get_debug_state().set_indent_level(curr + by);
}


void debug_indent_dec(int by)
{
	// indent level is thread-local, no need to lock anything
	int curr = debug_internal::get_debug_state().get_indent_level();
	curr -= by;
	if (curr < 0)
		curr = 0;
	debug_internal::get_debug_state().set_indent_level(curr);
}


void debug_indent_reset()
{
	// indent level is thread-local, no need to lock anything
	debug_internal::get_debug_state().set_indent_level(0);
}




namespace debug_internal {


	// manupulator objects
	DebugIndent debug_indent;
	DebugUnindent debug_unindent;
	DebugResetIndent debug_resindent;


}







