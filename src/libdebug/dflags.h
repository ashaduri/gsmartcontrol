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

#ifndef LIBDEBUG_DFLAGS_H
#define LIBDEBUG_DFLAGS_H

#include <bitset>


/// Debug level enum and related functions
namespace debug_level {
	enum flag : std::size_t {  ///< Debug level (seriousness). Some of these flags can be ORed for some functions.
		none = 0,  ///< No flags
		dump = 1 << 0,  ///< Dump level (structure dumps, additional verbosity, etc...)
		info = 1 << 1,  ///< Information level (what the application is doing)
		warn = 1 << 2,  ///< Warning level (simple warnings)
		error = 1 << 3,  ///< Error level (recoverable errors)
		fatal = 1 << 4,  ///< Fatal level (non-recoverable errors)
		all = dump | info | warn | error | fatal,  ///< All flags
		bits = 5  ///< Number of bits for bitset
	};

	using types = std::bitset<bits>;  ///< Combination of debug level flags

	/// Get debug level name
	const char* get_name(flag level);

	/// Get color start sequence for debug level (for colorizing the output)
	const char* get_color_start(flag level);

	/// Get color stop sequence for debug level (for colorizing the output)
	const char* get_color_stop(flag level);


	/// Convert ORed flags into a vector of flags
	template<class Container> inline
	void get_matched_levels_array(const types& levels, Container& put_here)
	{
		for (auto level : {
				debug_level::dump,
				debug_level::info,
				debug_level::warn,
				debug_level::error,
				debug_level::fatal }) {
			if (levels.test(level)) {
				put_here.push_back(level);
			}
		}
	}
}



/// Debug formatting option (how to format the message) enum and related functions
namespace debug_format {
	enum flag : std::size_t {  ///< Format flag. Some of these flags can be ORed for some functions.
		none = 0,  ///< No flags
		datetime = 1 << 0,  ///< Show datetime
		level = 1 << 1,  ///< Show debug level name
		domain = 1 << 2,  ///< Show domain name
		color = 1 << 3,  ///< Colorize output. Note: Colorization works only for supported shells / terminals (e.g. bash / xterm).
		indent = 1 << 4,  ///< Enable indentation
		first_line_only = 1 << 5,  ///< Internal flag, prefix first line only.
		all = datetime | level | domain | color | indent,  ///< All user formatting flags enabled
		bits = 6  ///< Number of bits for bitset
	};

	 using type = std::bitset<bits>;  ///< Combination of format flags
}



/// Debug position output flags (how to format the current source line information)
namespace debug_pos {
	enum flag : std::size_t {  ///< Position output flags
		none = 0,  ///< No flags
		func_name = 1 << 0,  ///< Print function name (only)
		func = 1 << 1,  ///< Print function name with namespaces, etc... (off by default).
		line = 1 << 2,  ///< Print source code line
		file = 1 << 3,  ///< Print file path and name
		def = func_name | line | file,  ///< Default flags
		bits = 4  ///< Number of bits for bitset
	};

	using type = std::bitset<bits>;  ///< Combination of flags
}







#endif

/// @}
