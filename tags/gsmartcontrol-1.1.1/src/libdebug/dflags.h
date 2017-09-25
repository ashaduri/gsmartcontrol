/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
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
	enum flag {  ///< Debug level (seriousness). Some of these flags can be ORed for some functions.
		none = 0,  ///< No flags
		dump = 1 << 0,  ///< Dump level (structure dumps, additional verbosity, etc...)
		info = 1 << 1,  ///< Information level (what the application is doing)
		warn = 1 << 2,  ///< Warning level (simple warnings)
		error = 1 << 3,  ///< Error level (recoverable errors)
		fatal = 1 << 4,  ///< Fatal level (non-recoverable errors)
		all = dump | info | warn | error | fatal,  ///< All flags
		bits = 5  ///< Number of bits for bitset
	};

	typedef std::bitset<bits> type;  ///< Combination of debug level flags

	/// Get debug level name
	const char* get_name(flag level);

	/// Get color start sequence for debug level (for colorizing the output)
	const char* get_color_start(flag level);

	/// Get color stop sequence for debug level (for colorizing the output)
	const char* get_color_stop(flag level);


	/// Convert ORed flags into a vector of flags
	template<class Container> inline
	void get_matched_levels_array(const type& levels, Container& put_here)
	{
		unsigned long levels_ulong = levels.to_ulong();
		if (levels_ulong & debug_level::dump) put_here.push_back(debug_level::dump);
		if (levels_ulong & debug_level::info) put_here.push_back(debug_level::info);
		if (levels_ulong & debug_level::warn) put_here.push_back(debug_level::warn);
		if (levels_ulong & debug_level::error) put_here.push_back(debug_level::error);
		if (levels_ulong & debug_level::fatal) put_here.push_back(debug_level::fatal);
	}
}



/// Debug formatting option (how to format the message) enum and related functions
namespace debug_format {
	enum flag {  ///< Format flag. Some of these flags can be ORed for some functions.
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

	typedef std::bitset<bits> type;  ///< Combination of format flags
}



/// Debug position output flags (how to format the current source line information)
namespace debug_pos {
	enum flag {  ///< Position output flags
		none = 0,  ///< No flags
		func_name = 1 << 0,  ///< Print function name (only)
		func = 1 << 1,  ///< Print function name with namespaces, etc... (off by default).
		line = 1 << 2,  ///< Print source code line
		file = 1 << 3,  ///< Print file path and name
		def = func_name | line | file,  ///< Default flags
		bits = 4  ///< Number of bits for bitset
	};

	typedef std::bitset<bits> type;  ///< Combination of flags
}







#endif

/// @}
