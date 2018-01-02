/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz_examples
/// \weakgroup hz_examples
/// @{

// disable libdebug, we don't link to it
#undef HZ_USE_LIBDEBUG
#define HZ_USE_LIBDEBUG 0
// enable libdebug emulation through std::cerr
#undef HZ_EMULATE_LIBDEBUG
#define HZ_EMULATE_LIBDEBUG 1

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "fs_path.h"
#include "fs_file.h"

#include <iostream>
#include <vector>



/// Main function for the test
int main()
{
	using namespace hz;


	{
		std::string s = "\\\\\\\\asd\\www";
		std::cerr << FsPath(s).to_native().str() << "\n";
	}


	{
		File p("Makeafile", "rb");
		if (p.bad()) {
			std::cerr << p.get_error_locale() << "\n";
		}
	}


	{
		std::string s;
		File p("Makeafile");
		if (p.get_contents(s)) {
			std::cerr << "Read " << s.size() << " bytes.\n";
		} else {
			std::cerr << p.get_error_locale() << "\n";
		}
	}


	{
		std::string s = "\\12/2$&! a23412";
		std::cerr << "Safe file: " << filename_make_safe(s)
				<< ", safe path " << path_make_safe(s) << "\n";
	}


	{
		std::vector<std::string> paths;

#ifdef _WIN32
		paths.push_back("A:\\temp\\ab");  // dir: A:\temp, base: ab
		paths.push_back("B:\\temp\\ab\\\\");  // dir: B:\temp, base: ab
		paths.push_back("\\\\host\\");  // dir: \\host\, base: \\host\ ;
		paths.push_back("C:\\");  // dir: C:\, base: C:\;
		paths.push_back("D:\\a\\\\b\\\\c");  // dir: D:\a\\b, base: c
		paths.push_back("\\a\\b\\c");  // dir: \a\b, base: c
		paths.push_back("d\\e\\f");  // dir: \d\e, base: f
		paths.push_back("\\f");  // dir: ., base: f; We can't use the \-paths, because there's no "current" drive.
		paths.push_back("g");  // dir: ., base: g
		paths.push_back("C:\\temp");  // dir: temp, base: C:\ ;
		paths.push_back("C:\\temp\\");  // dir: temp, base: C:\ ;
		paths.push_back("C:\\Documents and Settings\\whatever\\My Documents\\hello.conf");

		paths.push_back(".");  // dir: ., base: .
		paths.push_back("..");  // dir: ., base: ..
		paths.push_back("");  // dir: ., base: .


#else
		paths.push_back("C:\\22da\\a\\");
		paths.push_back("/usr/local/bin//");  // dir: /usr/local, base: bin
		paths.push_back("/a/dd//e/");  // dir: /a/dd, base: e
		paths.push_back("a/b/c/d/");  // dir: a/b/c, base: d

		// examples from man 2 dirname
		paths.push_back("/usr/local/lib");  // dir: /usr/local, base: lib
		paths.push_back("/usr/"); // dir: /, base: usr
		paths.push_back("usr"); // dir: ., base: usr
		paths.push_back("/"); // dir: /, base: /
		paths.push_back("."); // dir: ., base: .
		paths.push_back(".."); // dir: ., base: ..
		paths.push_back("");  // dir: ., base: .

		paths.push_back("./hello/a/b.././c/d/../e");
		paths.push_back("/a/../.././../b/");
		paths.push_back("../a/./b/..");
		paths.push_back("//.programrc");


#endif

		for (unsigned int i = 0; i < paths.size(); ++i) {
			FsPath p = paths[i];
			std::cerr << p.str() << ":\n"
				<< "\t" << "dir: " << p.get_dirname() << ", "
				<< " " << "base: " << p.get_basename() << ", "
				<< " " << "root: " << p.get_root() << "\n"
				<< "\t" << "abs: " << p.is_absolute() << ", "
				<< " " << "trim: " << path_trim_trailing_separators(p.str()) << ", "
				<< " " << "compress: " << path_compress(p.str()) << "\n";
		}
	}



	return 0;
}





/// @}
