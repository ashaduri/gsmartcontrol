/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_unlicense.txt
***************************************************************************/

// disable libdebug, we don't link to it
#undef HZ_USE_LIBDEBUG
#define HZ_USE_LIBDEBUG 0
// enable libdebug emulation through std::cerr
#undef HZ_EMULATE_LIBDEBUG
#define HZ_EMULATE_LIBDEBUG 1

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "format_unit.h"

#include <iostream>

#include "cstdint.h"




int main()
{
	std::cerr << hz::format_size(3 * 1024 * 1024) << "\n";  // 3 MiB
	std::cerr << hz::format_size(4LL * 1000 * 1000, true, true) << "\n";  // 4 Mbit
	std::cerr << hz::format_size(100LL * 1000 * 1000 * 1000) << "\n";  // 100 GB in GiB (aka how hard disk manufactures screw you)
	std::cerr << hz::format_size(100LL * 1024 * 1024, true) << "\n";  // 100 MiB in decimal GB

	std::cerr << hz::format_size(5) << "\n";  // 5 bytes
	std::cerr << hz::format_size(6, true, true) << "\n";  // 6 bits
	std::cerr << hz::format_size(uint64_t(2.5 * 1024)) << "\n";  // 2.5 KiB
	std::cerr << hz::format_size(uint64_t(2.5 * 1024 * 1024)) << "\n";  // 2.5 MiB
	std::cerr << hz::format_size(uint64_t(2.5 * 1024 * 1024 * 1024)) << "\n";  // 2.5 GiB
	std::cerr << hz::format_size(uint64_t(2.5 * 1024 * 1024 * 1024 * 1024)) << "\n";  // 2.5 TiB
	std::cerr << hz::format_size(uint64_t(2.5 * 1024 * 1024 * 1024 * 1024 * 1024)) << "\n";  // 2.5 PiB
	std::cerr << hz::format_size(uint64_t(2.5 * 1024 * 1024 * 1024 * 1024 * 1024 * 1024)) << "\n";  // 2.5 EiB

	std::cerr << hz::format_size(uint64_t(1000204886016ULL), true) << "\n";  // common size of 1 TiB hdd in decimal



	const uint64_t min_size = 60;
	const uint64_t hour_size = min_size * 60;
	const uint64_t day_size = hour_size * 24;

	std::cerr << hz::format_time_length(5) << "\n";  // 5 sec
	std::cerr << hz::format_time_length(uint64_t(5.5 * min_size)) << "\n";  // 5.5 min
	std::cerr << hz::format_time_length(130 * min_size) << "\n";  // 130 min
	std::cerr << hz::format_time_length(uint64_t(5.5 * hour_size)) << "\n";  // 5.5 hours
	std::cerr << hz::format_time_length(uint64_t(24.33 * hour_size)) << "\n";  // 24 hours, 20 minutes
	std::cerr << hz::format_time_length(130 * hour_size + 30 * min_size) << "\n";  // 130.5 hours
	std::cerr << hz::format_time_length(5 * day_size + 15 * hour_size + 30 * min_size) << "\n";  // 5 days, 15 hours, 30 minutes
	std::cerr << hz::format_time_length(20 * day_size - 8 * hour_size) << "\n";  // 19 days, 16 hours


	return 0;
}




