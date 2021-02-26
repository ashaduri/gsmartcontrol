/******************************************************************************
License: BSD Zero Clause License
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib_examples
/// \weakgroup applib_examples
/// @{

#undef HZ_USE_LIBDEBUG
#define HZ_USE_LIBDEBUG 0
// enable libdebug emulation through std::cerr
#undef HZ_EMULATE_LIBDEBUG
#define HZ_EMULATE_LIBDEBUG 1

#include <iostream>
#include <cstdlib>

#include "libdebug/libdebug.h"
#include "hz/fs.h"
#include "storage_property.h"
#include "smartctl_parser.h"



/// Main function of the test
int main(int argc, char** argv)
{
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " <file_to_parse>\n";
		return EXIT_FAILURE;
	}
	debug_register_domain("app");

	hz::fs::path file(argv[1]);  // native encoding
	std::string contents;
	auto ec = hz::fs_file_get_contents(file, contents, 10*1024*1024);  // 10M
	if (ec) {
		debug_out_error("app", ec.message() << "\n");
		return EXIT_FAILURE;
	}

	SmartctlParser sp;
	if (!sp.parse_full(contents, StorageAttribute::DiskType::Any)) {
		debug_out_error("app", "Cannot parse file contents: " << sp.get_error_msg() << "\n");
		return EXIT_FAILURE;
	}

	const std::vector<StorageProperty>& props = sp.get_properties();
	for(const auto& prop : props) {
		debug_out_dump("app", prop << "\n");
	}

	return EXIT_SUCCESS;
}





/// @}
