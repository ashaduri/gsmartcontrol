/******************************************************************************
License: Zlib
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_FORMAT_UNIT_H
#define HZ_FORMAT_UNIT_H

#include <string>
#include <cstddef>  // std::size_t
#include <ctime>  // for time.h, std::time, std::localtime, ...
#include <time.h>  // localtime_r, localtime_s
#include <iomanip>  // std::put_time
#include <cstdint>
#include <sstream>
#include <chrono>
#include <vector>

#if defined __MINGW32__
	#include <_mingw.h>  // MINGW_HAS_SECURE_API
#endif

#ifdef ENABLE_GLIB
	#include "local_glibmm.h"
	// #include <glib/gi18n.h>  // may cause conflicts with std::*printf()
#else
	#define C_(Context,String) (String)
#endif

#include "string_num.h"  // hz::number_to_string_locale
#include "string_algo.h"


/// \def HAVE_REENTRANT_LOCALTIME
/// Defined to 0 or 1. If 1, localtime() is reentrant.
#ifndef HAVE_REENTRANT_LOCALTIME
// win32 and solaris localtime() is reentrant
	#if defined _WIN32 || defined sun || defined __sun
		#define HAVE_REENTRANT_LOCALTIME 1
	#else
		#define HAVE_REENTRANT_LOCALTIME 0
	#endif
#endif




namespace hz {




/// Format byte or bit size in human-readable way, e.g. KB, mb, etc...
/// Note that kilobit always means 1000 bits, there's no confusion with that (as opposed to kilobyte).
/// This function honors the SI rules, e.g. GiB for binary, GB for decimal (as defined by SI).
inline std::string format_size(uint64_t size, bool use_decimal = false, bool size_is_bits = false)
{
	const uint64_t multiplier = (use_decimal ? 1000 : 1024);
	const uint64_t kb_size = multiplier;  // kilo
	const uint64_t mb_size = kb_size * multiplier;  // mega
	const uint64_t gb_size = mb_size * multiplier;  // giga
	const uint64_t tb_size = gb_size * multiplier;  // tera
	const uint64_t pb_size = tb_size * multiplier;  // peta
	const uint64_t eb_size = pb_size * multiplier;  // exa

	// these are beyond the size of uint64_t.
// 	const uint64_t zb_size = eb_size * multiplier;  // zetta
// 	const uint64_t yb_size = zb_size * multiplier;  // yotta

	// Note: This won't work with runtime language change.
	static const std::vector<std::string> names = {
		C_("file_size", "%s B"),  // bytes decimal
		C_("file_size", "%s B"),  // bytes binary
		C_("file_size", "%s bit"),  // bits decimal
		C_("file_size", "%s bit"),  // bits binary. note: 2 bit, not 2 bits.

		C_("file_size", "%s KB"),
		C_("file_size", "%s KiB"),
		C_("file_size", "%s Kbit"),
		C_("file_size", "%s Kibit"),

		C_("file_size", "%s MB"),
		C_("file_size", "%s MiB"),
		C_("file_size", "%s Mbit"),
		C_("file_size", "%s Mibit"),

		C_("file_size", "%s GB"),
		C_("file_size", "%s GiB"),
		C_("file_size", "%s Gbit"),
		C_("file_size", "%s Gibit"),

		C_("file_size", "%s TB"),
		C_("file_size", "%s TiB"),
		C_("file_size", "%s Tbit"),
		C_("file_size", "%s Tibit"),

		C_("file_size", "%s PB"),
		C_("file_size", "%s PiB"),
		C_("file_size", "%s Pbit"),
		C_("file_size", "%s Pibit"),

		C_("file_size", "%s EB"),
		C_("file_size", "%s EiB"),
		C_("file_size", "%s Ebit"),
		C_("file_size", "%s Eibit")
	};

	const std::size_t addn = static_cast<std::size_t>(!use_decimal) + (static_cast<std::size_t>(size_is_bits) * 2);

	if (size >= eb_size) {  // exa
		return hz::string_replace_copy(names[(6 * 4) + addn], "%s",
				hz::number_to_string_locale(static_cast<long double>(size) / static_cast<long double>(eb_size), 2, true), 1);
	}

	if (size >= pb_size) {  // peta
		return hz::string_replace_copy(names[(5 * 4) + addn], "%s",
				hz::number_to_string_locale(static_cast<long double>(size) / static_cast<long double>(pb_size), 2, true), 1);
	}

	if (size >= tb_size) {  // tera
		return hz::string_replace_copy(names[(4 * 4) + addn], "%s",
				hz::number_to_string_locale(static_cast<long double>(size) / static_cast<long double>(tb_size), 2, true), 1);
	}

	if (size >= gb_size) {  // giga
		return hz::string_replace_copy(names[(3 * 4) + addn], "%s",
				hz::number_to_string_locale(static_cast<long double>(size) / static_cast<long double>(gb_size), 2, true), 1);
	}

	if (size >= mb_size) {  // mega
		return hz::string_replace_copy(names[(2 * 4) + addn], "%s",
				hz::number_to_string_locale(static_cast<long double>(size) / static_cast<long double>(mb_size), 2, true), 1);
	}

	if (size >= kb_size) {  // kilo
		return hz::string_replace_copy(names[(1 * 4) + addn], "%s",
				hz::number_to_string_locale(static_cast<long double>(size) / static_cast<long double>(kb_size), 2, true), 1);
	}

	return hz::string_replace_copy(names[(0 * 4) + addn], "%s", std::to_string(size));
}



/// Format time length (e.g. 330 seconds) in a human-readable manner
/// (e.g. "5 min 30 sec").
inline std::string format_time_length(std::chrono::seconds secs)
{
	using namespace std::literals;
	using day_unit = std::chrono::duration
			<int, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

	// don't use uints here - they bring bugs.
	const int64_t min_size = 60;
	const int64_t hour_size = min_size * 60;
	const int64_t day_size = hour_size * 24;

	if (secs >= 100h) {
		day_unit days = std::chrono::round<day_unit>(secs);
		std::chrono::seconds sec_diff = secs - days;  // difference between days and actual time (may be positive or negative)

		if (days.count() < 10) {  // if less than 10 days, display hours too

			// if there's more than half an hour missing from complete day, add a day.
			// then add half an hour and convert to hours.
			int64_t hours = ((sec_diff.count() < (-hour_size / 2) ? sec_diff.count() + day_size : sec_diff.count()) + hour_size / 2) / hour_size;
		    if (hours > 0 && sec_diff.count() < (-hour_size / 2))
		       days--;

			return hz::string_replace_array_copy(C_("time", "{days} d {hours} h"),
					std::vector<std::string>{"{days}", "{hours}"},
					std::vector<std::string>{std::to_string(days.count()), std::to_string(hours)});

		}
		// display days only
		return hz::string_replace_copy(C_("time", "{days} d"),
			"{days}", std::to_string(days.count()));
	}

	if (secs >= 100min) {
		auto hours = std::chrono::round<std::chrono::hours>(secs);
		std::chrono::seconds sec_diff = secs - hours;

		if (hours.count() < 10) {  // if less than 10 hours, display minutes too
			int64_t minutes = ((sec_diff.count() < (-min_size / 2) ? sec_diff.count() + hour_size : sec_diff.count()) + min_size / 2) / min_size;
			if (minutes > 0 && sec_diff.count() < (-min_size / 2))
				hours--;

			return hz::string_replace_array_copy(C_("time", "{hours} h {minutes} min"),
					std::vector<std::string>{"{hours}", "{minutes}"},
					std::vector<std::string>{std::to_string(hours.count()), std::to_string(minutes)});

		}
		// display hours only
		return std::to_string(hours.count()) + " " + "h";
	}

	if (secs >= 100s) {
		auto minutes = std::chrono::round<std::chrono::minutes>(secs);
		return hz::string_replace_copy(C_("time", "{minutes} min"),
				"{minutes}", std::to_string(minutes.count()));
	}

	return hz::string_replace_copy(C_("time", "{seconds} sec"),
			"{seconds}", std::to_string(secs.count()));
}








/// Format a date specified by \c ltmp (pointer to tm structure).
/// See std::put_time() documentation for format details. To print ISO datetime
/// use "%Y-%m-%d %H:%M:%S" (sometimes 'T' is used instead of space).
inline std::string format_date(const std::string& format, const struct std::tm* ltmp, bool use_classic_locale)
{
	if (!ltmp || format.empty())
		return {};

	std::ostringstream ss;
	if (!use_classic_locale) {
		ss.imbue(std::locale::classic());
	}
	ss << std::put_time(ltmp, format.c_str());
	return ss.str();
}



/// Format a date specified by \c timet (seconds since Epoch).
/// See strftime() documentation for format details.
inline std::string format_date(const std::string& format, std::time_t timet, bool use_classic_locale)
{
#if defined MINGW_HAS_SECURE_API || defined _MSC_VER
	struct std::tm ltm;
	if (localtime_s(&ltm, &timet) != 0)  // shut up msvc (it thinks std::localtime() is unsafe)
		return std::string();
	const struct std::tm* ltmp = &ltm;

#elif defined HAVE_REENTRANT_LOCALTIME && HAVE_REENTRANT_LOCALTIME
	const struct std::tm* ltmp = std::localtime(&timet);

#else
	struct std::tm ltm = {};
	if (!localtime_r(&timet, &ltm))  // use reentrant localtime_r (posix/bsd and related)
		return {};
	const struct std::tm* ltmp = &ltm;
#endif

	return format_date(format, ltmp, use_classic_locale);
}



/// Format current date.
/// See strftime() documentation for format details.
inline std::string format_date(const std::string& format, bool use_classic_locale)
{
	const std::time_t timet = std::time(nullptr);
	if (timet == static_cast<std::time_t>(-1))
		return {};

	return format_date(format, timet, use_classic_locale);
}






} // ns




#endif

/// @}
