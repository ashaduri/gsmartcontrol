/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#include <time.h>  // <ctime> doesn't contain ctime_r(), and std::ctime() is not thread-safe.

#include "dchannel.h"
// #include <iostream>  // tmp


// a helper function for DebugChannel-s. formats a message.
std::string debug_format_message(debug_level::flag level, const std::string& domain,
				debug_format::type& format_flags, int indent_level, bool is_first_line, const std::string& msg)
{
	if (msg.empty())
		return msg;

	std::string ret;
	ret.reserve(msg.size() + 40);  // indentation + domain/level

	// allow prefix only for first line and others when first_line_only is disabled
	if (is_first_line || !(format_flags.to_ulong() & debug_format::first_line_only)) {

		if (format_flags.to_ulong() & debug_format::datetime) {  // print time
			time_t t = time(0);
			if (t != -1) {
#ifndef _WIN32
				char buf[28] = {0}; // ctime_r requires max. 26 chars in buffer. align to 28.
				ctime_r(&t, buf);
#else
				const char* buf = ctime(&t);  // win32 doesn't have ctime_r(), but its ctime() is reentrant.
#endif
				if (*buf) {
					ret.append(buf, 24);
					ret += ": ";
				}
			}
		}


		if (format_flags.to_ulong() & debug_format::level) {  // print level name
			bool use_color = (format_flags.to_ulong() & debug_format::color);
			if (use_color)
				ret += debug_level::get_color_start(level);

			std::string level_name = std::string("<") + debug_level::get_name(level) + ">";
			ret += level_name + std::string(8 - level_name.size(), ' ');

			if (use_color)
				ret += debug_level::get_color_stop(level);
		}


		if (format_flags.to_ulong() & debug_format::domain) {  // print domain name
			ret += std::string("[") + domain + "] ";
		}

	}


	if (format_flags.to_ulong() & debug_format::indent) {
		std::string spaces(indent_level * 4, ' ');  // indentation spaces
// 		std::cerr << "MSG: " << msg;

		// replace all newlines with \n(indent-spaces) except for the last one.
		std::string::size_type oldpos = 0, pos = 0;
		while(pos != std::string::npos) {
			if (pos == 0) {
				ret.insert(0, spaces);
			} else {
				ret.append(spaces);
			}
			pos = msg.find('\n', oldpos);
			if (pos != std::string::npos)
				pos++;
			if (pos >= msg.size())
				pos = std::string::npos;

			ret.append(msg, oldpos, pos - oldpos);
			oldpos = pos;

// 			pos = std::string::npos;
// 			ret.append(msg);
		}

	} else {
		ret += msg;
	}

// 	std::cerr << "FINAL MSG: " << ret;


	return ret;
}







