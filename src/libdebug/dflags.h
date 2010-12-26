/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef LIBDEBUG_DFLAGS_H
#define LIBDEBUG_DFLAGS_H

#include <bitset>


namespace debug_level {
	enum flag {
		none = 0,
		dump = 1 << 0,
		info = 1 << 1,
		warn = 1 << 2,
		error = 1 << 3,
		fatal = 1 << 4,
		all = dump | info | warn | error | fatal,
		bits = 5
	};

	typedef std::bitset<bits> type;

	const char* get_name(flag level);

	const char* get_color_start(flag level);
	const char* get_color_stop(flag level);


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



namespace debug_format {
	enum flag {
		none = 0,
		datetime = 1 << 0,  // show datetime
		level = 1 << 1,  // show level name
		domain = 1 << 2,  // show domain name
		color = 1 << 3,  // note: colorification works only for supported shells / terminals (e.g. bash / xterm).
		indent = 1 << 4,  // enable indentation
		first_line_only = 1 << 5,  // internal. prefix first line only.
		all = datetime | level | domain | color | indent,
		bits = 6
	};

	typedef std::bitset<bits> type;
}




namespace debug_pos {

	enum flag {
		none = 0,
		func_name = 1 << 0,
		func = 1 << 1,  // off by default.
		line = 1 << 2,
		file = 1 << 3,
		def = func_name | line | file,
		bits = 4
	};

	typedef std::bitset<bits> type;
}







#endif
