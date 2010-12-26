/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

// disable libdebug, we don't link to it
#undef HZ_USE_LIBDEBUG
#define HZ_USE_LIBDEBUG 0
// enable libdebug emulation through std::cerr
#undef HZ_EMULATE_LIBDEBUG
#define HZ_EMULATE_LIBDEBUG 1

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "fs_file.h"

#include <iostream>
#include <vector>



int main()
{
	std::vector<std::string> files;
	files.push_back("/usr/bin/ar");
	files.push_back("/proc/partitions");

	for (unsigned int i = 0; i < files.size(); ++i) {
		hz::File file(files[i]);

		hz::file_size_t size1 = 0, size2 = 0;

		file.get_size(size1);  // stat() method
		file.get_size(size2, true);  // fread() method

		std::cerr << "File " << file.str() << "\n";
		std::cerr << "stat size: " << size1 << ", fread size: " << size2 << "\n\n";
	}

	return 0;
}




