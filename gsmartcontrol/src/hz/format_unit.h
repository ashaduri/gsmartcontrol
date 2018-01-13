/**************************************************************************
 Copyright:
      (C) 2003 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_FORMAT_UNIT_H
#define HZ_FORMAT_UNIT_H

#include "hz_config.h"  // feature macros

#include <string>
#include <cstddef>  // std::size_t
#include <ctime>  // for time.h, std::time, std::localtime, ...
#include <iomanip>  // std::put_time
#include <cstdint>
#include <sstream>

#include "string_num.h"  // hz::number_to_string_locale
#include "i18n.h"  // HZ_NC_, HZ_C_


// HAVE_WIN_SE_FUNCS means localtime_s
#if (defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS) \
		|| !(defined HAVE_REENTRANT_LOCALTIME && HAVE_REENTRANT_LOCALTIME)
	#include <time.h>  // localtime_r, localtime_s
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

	static const char* const names[] = {
		HZ_NC_("filesize", " B"),  // bytes decimal
		HZ_NC_("filesize", " B"),  // bytes binary
		HZ_NC_("filesize", " bit"),  // bits decimal
		HZ_NC_("filesize", " bit"),  // bits binary. note: 2 bit, not 2 bits.

		HZ_NC_("filesize", " KB"),
		HZ_NC_("filesize", " KiB"),
		HZ_NC_("filesize", " Kbit"),
		HZ_NC_("filesize", " Kibit"),

		HZ_NC_("filesize", " MB"),
		HZ_NC_("filesize", " MiB"),
		HZ_NC_("filesize", " Mbit"),
		HZ_NC_("filesize", " Mibit"),

		HZ_NC_("filesize", " GB"),
		HZ_NC_("filesize", " GiB"),
		HZ_NC_("filesize", " Gbit"),
		HZ_NC_("filesize", " Gibit"),

		HZ_NC_("filesize", " TB"),
		HZ_NC_("filesize", " TiB"),
		HZ_NC_("filesize", " Tbit"),
		HZ_NC_("filesize", " Tibit"),

		HZ_NC_("filesize", " PB"),
		HZ_NC_("filesize", " PiB"),
		HZ_NC_("filesize", " Pbit"),
		HZ_NC_("filesize", " Pibit"),

		HZ_NC_("filesize", " EB"),
		HZ_NC_("filesize", " EiB"),
		HZ_NC_("filesize", " Ebit"),
		HZ_NC_("filesize", " Eibit")
	};

	const int addn = static_cast<int>(!use_decimal) + (static_cast<int>(size_is_bits) * 2);

	if (size >= eb_size) {  // exa
		return hz::number_to_string_locale(static_cast<long double>(size) / static_cast<long double>(eb_size), 2, true)
				+ HZ_RC_("filesize", names[(6 * 4) + addn]);
	}

	if (size >= pb_size) {  // peta
		return hz::number_to_string_locale(static_cast<long double>(size) / static_cast<long double>(pb_size), 2, true)
				+ HZ_RC_("filesize", names[(5 * 4) + addn]);
	}

	if (size >= tb_size) {  // tera
		return hz::number_to_string_locale(static_cast<long double>(size) / static_cast<long double>(tb_size), 2, true)
				+ HZ_RC_("filesize", names[(4 * 4) + addn]);
	}

	if (size >= gb_size) {  // giga
		return hz::number_to_string_locale(static_cast<long double>(size) / static_cast<long double>(gb_size), 2, true)
				+ HZ_RC_("filesize", names[(3 * 4) + addn]);
	}

	if (size >= mb_size) {  // mega
		return hz::number_to_string_locale(static_cast<long double>(size) / static_cast<long double>(mb_size), 2, true)
				+ HZ_RC_("filesize", names[(2 * 4) + addn]);
	}

	if (size >= kb_size) {  // kilo
		return hz::number_to_string_locale(static_cast<long double>(size) / static_cast<long double>(kb_size), 2, true)
				+ HZ_RC_("filesize", names[(1 * 4) + addn]);
	}

	return std::to_string(size) + HZ_RC_("filesize", names[(0 * 4) + addn]);
}



/// Format time length (e.g. 330 seconds) in a human-readable manner
/// (e.g. "5 min 30 sec").
inline std::string format_time_length(int64_t secs)
{
	// don't use uints here - there bring bugs.
	const int64_t min_size = 60;
	const int64_t hour_size = min_size * 60;
	const int64_t day_size = hour_size * 24;

	if (secs >= 100 * hour_size) {
		int64_t days = (secs + day_size / 2) / day_size;  // time in days (rounded to nearest)
		int64_t sec_diff = secs - days * day_size;  // difference between days and actual time (may be positive or negative)

		if (days < 10) {  // if less than 10 days, display hours too

			// if there's more than half an hour missing from complete day, add a day.
			// then add half an hour and convert to hours.
			int64_t hours = ((sec_diff < (-hour_size / 2) ? sec_diff + day_size : sec_diff) + hour_size / 2) / hour_size;
		    if (hours > 0 && sec_diff < (-hour_size / 2))
		       days--;

			return std::to_string(days) + " " + HZ_C_("time", "d")
					+ " " + std::to_string(hours) + " " + HZ_C_("time", "h");

		} else {  // display days only
			return std::to_string(days) + " " + HZ_C_("time", "d");
		}

	} else if (secs >= 100 * min_size) {
		int64_t hours = (secs + hour_size / 2) / hour_size;  // time in hours (rounded to nearest)
		int64_t sec_diff = secs - hours * hour_size;

		if (hours < 10) {  // if less than 10 hours, display minutes too
			int64_t minutes = ((sec_diff < (-min_size / 2) ? sec_diff + hour_size : sec_diff) + min_size / 2) / min_size;
			if (minutes > 0 && sec_diff < (-min_size / 2))
				hours--;

			return std::to_string(hours) + " " + HZ_C_("time", "h")
					+ " " + std::to_string(minutes) + " " + HZ_C_("time", "min");

		} else {  // display hours only
			return std::to_string(hours) + " " + HZ_C_("time", "h");
		}


	} else if (secs >= 100) {
		int64_t minutes = (secs + min_size / 2) / min_size;  // time in minutes (rounded to nearest)
		return std::to_string(minutes) + " " + HZ_C_("time", "min");
	}

	return std::to_string(secs) + " " + HZ_C_("time", "sec");
}








/// Format a date specified by \c ltmp (pointer to tm structure).
/// See std::put_time() documentation for format details. To print ISO datetime
/// use "%Y-%m-%d %H:%M:%S" (sometimes 'T' is used instead of space).
inline std::string format_date(const std::string& format, const struct std::tm* ltmp, bool use_classic_locale)
{
	if (!ltmp || format.empty())
		return std::string();

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
#if defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
	struct std::tm ltm;
	if (localtime_s(&ltm, &timet) != 0)  // shut up msvc (it thinks std::localtime() is unsafe)
		return std::string();
	const struct std::tm* ltmp = &ltm;

#elif defined HAVE_REENTRANT_LOCALTIME && HAVE_REENTRANT_LOCALTIME
	const struct std::tm* ltmp = std::localtime(&timet);

#else
	struct std::tm ltm;
	if (!localtime_r(&timet, &ltm))  // use reentrant localtime_r (posix/bsd and related)
		return std::string();
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
		return std::string();

	return format_date(format, timet, use_classic_locale);
}






} // ns




#endif

/// @}
