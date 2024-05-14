/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2022 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include "smartctl_text_parser_helper.h"
#include "build_config.h"
#include "hz/locale_tools.h"
#include "hz/format_unit.h"  // format_size
#include "hz/string_num.h"  // string_is_numeric, number_to_string
#include "hz/string_algo.h"  // string_*
#include "hz/debug.h"



std::string SmartctlTextParserHelper::parse_byte_size(std::string str, int64_t& bytes, bool extended)
{
	// E.g. "500,107,862,016" bytes or "80'060'424'192 bytes" or "80 026 361 856 bytes" or "750,156,374,016 bytes [750 GB]".
	// French locale inserts 0xA0 as a separator (non-breaking space, _not_ a valid utf8 char).
	// Finnish uses 0xC2 as a separator.
	// Added '.'-separated too, just in case.
	// Smartctl uses system locale's thousands_sep explicitly.

	// When launching smartctl, we use LANG=C for it, but it works only on POSIX.
	// Also, loading smartctl output files from different locales doesn't really work.

	str = str.substr(0, str.find('['));

	std::vector<std::string> to_replace = {
			" ",
			"'",
			",",
			".",
			std::string(1, static_cast<char>(0xa0)),
			std::string(1, static_cast<char>(0xc2)),
	};

	if constexpr(BuildEnv::is_kernel_family_windows()) {
		// if current locale is C, then probably we didn't change it at application
		// startup, so set it now (temporarily). Otherwise, just use the current locale's
		// thousands separator.
		{
			const std::string old_locale = hz::locale_c_get();
			const hz::ScopedCLocale loc("", old_locale == "C");  // set system locale if the current one is C

			struct lconv* lc = std::localeconv();
			if (lc && lc->thousands_sep && lc->thousands_sep[0] != '\0') {
				to_replace.emplace_back(lc->thousands_sep);
			}
		}  // the locale is restored here
	}

	to_replace.emplace_back("bytes");
	str = hz::string_replace_array_copy(hz::string_trim_copy(str), to_replace, "");

	int64_t v = 0;
	if (hz::string_is_numeric_nolocale(str, v, false)) {
		bytes = v;
		return hz::format_size(static_cast<uint64_t>(v), true) + (extended ?
				" [" + hz::format_size(static_cast<uint64_t>(v), false) + ", " + hz::number_to_string_locale(v) + " bytes]" : "");
	}

	return {};
}




/// @}




