/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef APP_PCRECPP_H
#define APP_PCRECPP_H

#include <pcrecpp.h>
#include <string>
#include <string_view>

#include "hz/debug.h"


/**
 * \file
 * Convenience wrappers for pcrecpp
 */



/// Take a string of characters where each character represents a modifier
/// and return the appropriate pcre options.
/// - i - case insensitive match.
/// - m - multiline, read past the first line.
/// - s - dot matches newlines.
/// - E - $ matches only the end of the string (D in php, not available in perl).
/// - X - strict escape parsing (not available in perl).
/// - x - ignore whitespaces.
/// - 8 - handles UTF8 characters in pattern (u in php, not available in perl).
/// - U - ungreedy, reverses * and *? (not available in perl).
/// - N - disables matching parentheses (not available in perl or php).
inline pcrecpp::RE_Options app_pcre_get_options(std::string_view modifiers);



/// Create a regular expression.
/// Pattern is in form of "/pattern/modifiers".
/// Note: Slashes should be escaped with backslashes within the pattern.
/// If the string doesn't start with a slash, it is treated as an ordinary pattern
/// without modifiers.
/// This function will make a RE object with ANYCRLF option set for portability
/// across various pcre builds.
inline pcrecpp::RE app_pcre_re(const std::string& perl_pattern);



/// Partially match a string against a regular expression.
/// matched_ptr arguments take pointers to std::string and arithmetical types,
/// and are filled with matched parts of `str.
/// \return true if a match was found.
inline bool app_pcre_match(const pcrecpp::RE& re, const std::string_view& str,
		const pcrecpp::Arg& matched_ptr1 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr2 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr3 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr4 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr5 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr6 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr7 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr8 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr9 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr10 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr11 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr12 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr13 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr14 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr15 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr16 = pcrecpp::RE::no_arg);



/// Partially match a string against a pattern in "/pattern/modifiers" format.
/// matched_ptr arguments take pointers to std::string and arithmetical types,
/// and are filled with matched parts of `str.
/// \return true if a match was found.
inline bool app_pcre_match(const std::string& perl_pattern, const std::string_view& str,
		const pcrecpp::Arg& matched_ptr1 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr2 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr3 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr4 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr5 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr6 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr7 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr8 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr9 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr10 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr11 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr12 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr13 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr14 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr15 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr16 = pcrecpp::RE::no_arg);



/// Match a string against a pattern in "/pattern/modifiers" format.
/// matched_ptr arguments take pointers to std::string and arithmetical types,
/// and are filled with matched parts of `str.
/// This overload is needed to avoid ambiguity with pcrecpp::RE overload.
/// \return true if a match was found.
inline bool app_pcre_match(const char* perl_pattern, const std::string_view& str,
		const pcrecpp::Arg& matched_ptr1 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr2 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr3 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr4 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr5 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr6 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr7 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr8 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr9 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr10 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr11 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr12 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr13 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr14 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr15 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& matched_ptr16 = pcrecpp::RE::no_arg);



/// Replace every occurrence of pattern with replacement string in \c subject.
/// \return number of replacements made.
inline int app_pcre_replace(const pcrecpp::RE& re, const std::string_view& replacement, std::string& subject);


/// Replace every occurrence of pattern with replacement string in \c subject.
/// The pattern is in "/pattern/modifiers" format.
/// \return number of replacements made.
inline int app_pcre_replace(const std::string& perl_pattern, const std::string_view& replacement, std::string& subject);


/// Replace every occurrence of pattern with replacement string in \c subject.
/// The pattern is in "/pattern/modifiers" format.
/// \return number of replacements made.
inline int app_pcre_replace(const char* perl_pattern, const std::string_view& replacement, std::string& subject);



/// Replace first occurrence of pattern with replacement string in \c subject.
/// \return true if a replacement was made
inline bool app_pcre_replace_once(const pcrecpp::RE& re, const std::string_view& replacement, std::string& subject);


/// Replace the first occurrence of pattern with replacement string in \c subject.
/// The pattern is in "/pattern/modifiers" format.
/// \return true if a replacement was made
inline bool app_pcre_replace_once(const std::string& perl_pattern, const std::string_view& replacement, std::string& subject);


/// Replace the first occurrence of pattern with replacement string in \c subject.
/// The pattern is in "/pattern/modifiers" format.
/// \return true if a replacement was made
inline bool app_pcre_replace_once(const char* perl_pattern, const std::string_view& replacement, std::string& subject);



/// Escape a string to be used inside a regular expression. The result
/// won't contain any special expression characters.
/// Note that this function escapes any non-alphanumeric character (including slash),
/// as this is allowed by PCRE.
inline std::string app_pcre_escape(const std::string_view& str);




// ------------------------------------------- Implementation



pcrecpp::RE_Options app_pcre_get_options(std::string_view modifiers)
{
	// ANYCRLF means any of crlf, cr, lf. Used in ^ and $.
	// This overrides the build-time newline setting of pcre.
#ifdef PCRE_NEWLINE_ANYCRLF  // not available in older versions of pcre1, not wrapped in pcrecpp
	pcrecpp::RE_Options options(PCRE_NEWLINE_ANYCRLF);
#else
	pcrecpp::RE_Options options;
#endif

	for (char c : modifiers) {
		switch (c) {
			// Note: Most of these are from pcretest man page.
			// Perl lacks some of them.
			case 'i': options.set_caseless(true); break;  // case insensitive match.
			case 'm': options.set_multiline(true); break;  // read past the first line too.
			case 's': options.set_dotall(true); break;  // dot matches newlines.
			case 'E': options.set_dollar_endonly(true); break;  // not in perl. php has D. $ matches only at end.
			case 'X': options.set_extra(true); break;  // not in perl. strict escape parsing
			case 'x': options.set_extended(true); break;  // ignore whitespaces
			case '8': options.set_utf8(true); break;  // not in perl. php has u. handles UTF8 chars in pattern.
			case 'U': options.set_ungreedy(true); break;  // not in perl. reverses * and *?
			case 'N': options.set_no_auto_capture(true); break;  // not in perl or php. disables matching parentheses.
			default: debug_out_error("app", DBG_FUNC_MSG << "Unknown modifier \'" << c << "\'\n"); break;
		}
	}

	return options;
}



pcrecpp::RE app_pcre_re(const std::string& perl_pattern)
{
	if (perl_pattern.size() >= 2 && perl_pattern[0] == '/') {

		// find the separator
		std::string::size_type endpos = perl_pattern.rfind('/');
		DBG_ASSERT(endpos != std::string::npos);  // shouldn't happen

		// no need to unescape slashes in pattern - pcre seems to not mind.
		return pcrecpp::RE(perl_pattern.substr(1, endpos - 1),
				app_pcre_get_options(perl_pattern.substr(endpos + 1)));
	}

	return pcrecpp::RE(perl_pattern, app_pcre_get_options({}));
}



bool app_pcre_match(const pcrecpp::RE& re, const std::string_view& str,
		const pcrecpp::Arg& matched_ptr1,
		const pcrecpp::Arg& matched_ptr2,
		const pcrecpp::Arg& matched_ptr3,
		const pcrecpp::Arg& matched_ptr4,
		const pcrecpp::Arg& matched_ptr5,
		const pcrecpp::Arg& matched_ptr6,
		const pcrecpp::Arg& matched_ptr7,
		const pcrecpp::Arg& matched_ptr8,
		const pcrecpp::Arg& matched_ptr9,
		const pcrecpp::Arg& matched_ptr10,
		const pcrecpp::Arg& matched_ptr11,
		const pcrecpp::Arg& matched_ptr12,
		const pcrecpp::Arg& matched_ptr13,
		const pcrecpp::Arg& matched_ptr14,
		const pcrecpp::Arg& matched_ptr15,
		const pcrecpp::Arg& matched_ptr16)
{
	return re.PartialMatch({str.data(), static_cast<int>(str.size())},
			matched_ptr1,
			matched_ptr2,
			matched_ptr3,
			matched_ptr4,
			matched_ptr5,
			matched_ptr6,
			matched_ptr7,
			matched_ptr8,
			matched_ptr9,
			matched_ptr10,
			matched_ptr11,
			matched_ptr12,
			matched_ptr13,
			matched_ptr14,
			matched_ptr15,
			matched_ptr16);
}



bool app_pcre_match(const std::string& perl_pattern, const std::string_view& str,
		const pcrecpp::Arg& matched_ptr1,
		const pcrecpp::Arg& matched_ptr2,
		const pcrecpp::Arg& matched_ptr3,
		const pcrecpp::Arg& matched_ptr4,
		const pcrecpp::Arg& matched_ptr5,
		const pcrecpp::Arg& matched_ptr6,
		const pcrecpp::Arg& matched_ptr7,
		const pcrecpp::Arg& matched_ptr8,
		const pcrecpp::Arg& matched_ptr9,
		const pcrecpp::Arg& matched_ptr10,
		const pcrecpp::Arg& matched_ptr11,
		const pcrecpp::Arg& matched_ptr12,
		const pcrecpp::Arg& matched_ptr13,
		const pcrecpp::Arg& matched_ptr14,
		const pcrecpp::Arg& matched_ptr15,
		const pcrecpp::Arg& matched_ptr16)
{
	return app_pcre_match(app_pcre_re(perl_pattern), str,
			matched_ptr1,
			matched_ptr2,
			matched_ptr3,
			matched_ptr4,
			matched_ptr5,
			matched_ptr6,
			matched_ptr7,
			matched_ptr8,
			matched_ptr9,
			matched_ptr10,
			matched_ptr11,
			matched_ptr12,
			matched_ptr13,
			matched_ptr14,
			matched_ptr15,
			matched_ptr16);
}



bool app_pcre_match(const char* perl_pattern, const std::string_view& str,
		const pcrecpp::Arg& matched_ptr1,
		const pcrecpp::Arg& matched_ptr2,
		const pcrecpp::Arg& matched_ptr3,
		const pcrecpp::Arg& matched_ptr4,
		const pcrecpp::Arg& matched_ptr5,
		const pcrecpp::Arg& matched_ptr6,
		const pcrecpp::Arg& matched_ptr7,
		const pcrecpp::Arg& matched_ptr8,
		const pcrecpp::Arg& matched_ptr9,
		const pcrecpp::Arg& matched_ptr10,
		const pcrecpp::Arg& matched_ptr11,
		const pcrecpp::Arg& matched_ptr12,
		const pcrecpp::Arg& matched_ptr13,
		const pcrecpp::Arg& matched_ptr14,
		const pcrecpp::Arg& matched_ptr15,
		const pcrecpp::Arg& matched_ptr16)
{
	return app_pcre_match(app_pcre_re(perl_pattern), str,
			matched_ptr1,
			matched_ptr2,
			matched_ptr3,
			matched_ptr4,
			matched_ptr5,
			matched_ptr6,
			matched_ptr7,
			matched_ptr8,
			matched_ptr9,
			matched_ptr10,
			matched_ptr11,
			matched_ptr12,
			matched_ptr13,
			matched_ptr14,
			matched_ptr15,
			matched_ptr16);
}



int app_pcre_replace(const pcrecpp::RE& re, const std::string_view& replacement, std::string& subject)
{
	return re.GlobalReplace({replacement.data(), static_cast<int>(replacement.size())}, &subject);
}



int app_pcre_replace(const std::string& perl_pattern, const std::string_view& replacement, std::string& subject)
{
	return app_pcre_replace(app_pcre_re(perl_pattern), replacement, subject);
}



int app_pcre_replace(const char* perl_pattern, const std::string_view& replacement, std::string& subject)
{
	return app_pcre_replace(app_pcre_re(perl_pattern), replacement, subject);
}



bool app_pcre_replace_once(const pcrecpp::RE& re, const std::string_view& replacement, std::string& subject)
{
	return re.Replace({replacement.data(), static_cast<int>(replacement.size())}, &subject);
}



bool app_pcre_replace_once(const std::string& perl_pattern, const std::string_view& replacement, std::string& subject)
{
	return app_pcre_replace_once(app_pcre_re(perl_pattern), replacement, subject);
}



bool app_pcre_replace_once(const char* perl_pattern, const std::string_view& replacement, std::string& subject)
{
	return app_pcre_replace_once(app_pcre_re(perl_pattern), replacement, subject);
}



std::string app_pcre_escape(const std::string_view& str)
{
	// This escapes practically any non-alphanumeric character
	return pcrecpp::RE::QuoteMeta({str.data(), static_cast<int>(str.size())});
}



#endif

/// @}
