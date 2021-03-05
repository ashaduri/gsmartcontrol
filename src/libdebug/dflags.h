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
	enum flag : std::size_t {  ///< Debug level (seriousness).
		dump,  ///< Dump level (structure dumps, additional verbosity, etc...)
		info,  ///< Information level (what the application is doing)
		warn,  ///< Warning level (simple warnings)
		error,  ///< Error level (recoverable errors)
		fatal,  ///< Fatal level (non-recoverable errors)
		bits,  ///< Number of bits for bitset
	};

	using flags = std::bitset<bits>;  ///< Combination of debug level flags

	/// Get bitset with all flags enabled
	const flags& get_all_flags();

	/// Get debug level name
	const char* get_name(flag level);

	/// Get color start sequence for debug level (for colorizing the output)
	const char* get_color_start(flag level);

	/// Get color stop sequence for debug level (for colorizing the output)
	const char* get_color_stop(flag level);


	/// Convert ORed flags into a vector of flags
	template<class Container> inline
	void get_matched_levels_array(const flags& levels, Container& put_here)
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
		datetime,  ///< Show datetime
		level,  ///< Show debug level name
		domain,  ///< Show domain name
		color,  ///< Colorize output. Note: Colorization works only for supported shells / terminals (e.g. bash / xterm).
		indent,  ///< Enable indentation
		first_line_only,  ///< Internal flag, prefix first line only.
		bits,  ///< Number of bits for bitset
	};

	 using flags = std::bitset<bits>;  ///< Combination of format flags
}



/// Debug position output flags (how to format the current source line information)
namespace debug_pos {
	enum flag : std::size_t {  ///< Position output flags
		func_name,  ///< Print function name (only)
		func,  ///< Print function name with namespaces, etc... (off by default).
		line,  ///< Print source code line
		file,  ///< Print file path and name
		// def = func_name | line | file,  ///< Default flags
		bits,  ///< Number of bits for bitset
	};

	using flags = std::bitset<bits>;  ///< Combination of flags
}







#endif

/// @}
