/**************************************************************************
 Copyright:
      (C) 2009 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_FS_FSTREAM_H
#define HZ_FS_FSTREAM_H

#include "hz_config.h"  // feature macros

// #include <fstream>

// #include "hz/win32_tools.h"  // win32_utf8_to_utf16, win32_utf16_to_utf8.


// The problem with std::fstream is the following:
// Under windows (what else), fstream (and fopen(), which libstdc++'s fstream
// is implemented upon) accepts filenames in user's current locale encoding.
// The problem is that the filesystem stores filenames in utf-16, and not all
// utf-16 filenames are representable through user's locale encoding. This means
// that not all files are openable through fstream/fopen()!
// The MS fstream implementation has additional open() method which accepts
// wchar_t* strings (in utf-16, that is). However, there's no such method in mingw.
// So, the only way to open all files on windows is to use _wfopen().


namespace hz {

/*
class ifstream : public std::ifstream {

	public:

		ifstream()
		{ }

		// file must be in utf-8 format (in win32).
		explicit ifstream(const char* file, std::ios_base::openmode mode = ios_base::in)
		{
			fs.open(file, mode);
		}

		// file must be in utf-8 format (in win32). this is a C++0x addition.
		explicit ifstream(const std::string& file, std::ios_base::openmode mode = ios_base::in)
		{
			fs.open(file, mode);
		}

		virtual ~ifstream()
		{ }

		// file must be in utf-8 format (in win32).
		bool open(const char* file, std::ios_base::openmode mode = ios_base::in)
		{
			std::ifstream::open(file);
		}

		// file must be in utf-8 format (in win32). this is a C++0x addition.
		bool open(const std::string& file, std::ios_base::openmode mode = ios_base::in)
		{
			std::ifstream::open(file);
		}

};

*/






}  // ns hz



#endif
