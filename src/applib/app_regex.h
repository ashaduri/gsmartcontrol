/******************************************************************************
 License: GNU General Public License v3.0 only
 Copyright:
 	(C) 2024 Alexander Shaduri <ashaduri@gmail.com>
 ******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef APP_REGEX_H
#define APP_REGEX_H

#include <string>
#include <string_view>
#include <vector>
#include <regex>

#include "hz/debug.h"


/**
 * \file
 * Convenience wrappers for std::regex
 */



/// Take a string of characters where each character represents a modifier
/// and return the appropriate flags.
/// - i - case insensitive match.
/// - m - multiline (EcmaScript syntax only).
inline std::regex::flag_type app_regex_get_options(std::string_view modifiers)
{
	std::regex::flag_type options = std::regex::ECMAScript;  // default, non-greedy.
	for (const char c : modifiers) {
		switch (c) {
			case 'i': options |= std::regex_constants::icase; break;  // case insensitive match.
			case 'm': options |= std::regex_constants::multiline; break;  // ^ and $ match each line as well. EcmaScript only.
			default: debug_out_error("app", DBG_FUNC_MSG << "Unknown modifier \'" << c << "\'\n"); break;
		}
	}
	return options;
}



/// Create a regular expression.
/// Pattern is in form of "/pattern/modifiers".
/// Note: Slashes should be escaped with backslashes within the pattern.
/// If the string doesn't start with a slash, it is treated as an ordinary pattern
/// without modifiers.
inline std::regex app_regex_re(const std::string& perl_pattern)
{
	if (perl_pattern.size() >= 2 && perl_pattern[0] == '/') {

		// find the separator
		const std::string::size_type endpos = perl_pattern.rfind('/');
		DBG_ASSERT(endpos != std::string::npos);  // shouldn't happen

		// no need to unescape slashes in pattern - pcre seems to not mind.
		return std::regex(perl_pattern.substr(1, endpos - 1),
				app_regex_get_options(perl_pattern.substr(endpos + 1)));
	}

	return std::regex(perl_pattern, app_regex_get_options({}));
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_partial_match(const std::regex& re, const std::string& str)
{
	return std::regex_search(str, re);
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_partial_match(const std::string& perl_pattern, const std::string& str)
{
	return app_regex_partial_match(app_regex_re(perl_pattern), str);
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_partial_match(const char* perl_pattern, const std::string& str)
{
	return app_regex_partial_match(app_regex_re(perl_pattern), str);
}



/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_partial_match(const std::regex& re, const std::string& str, std::smatch& matches)
{
	return std::regex_search(str, matches, re);
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_partial_match(const std::string& perl_pattern, const std::string& str, std::smatch& matches)
{
	return app_regex_partial_match(app_regex_re(perl_pattern), str, matches);
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_partial_match(const char* perl_pattern, const std::string& str, std::smatch& matches)
{
	return app_regex_partial_match(app_regex_re(perl_pattern), str, matches);
}



/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_partial_match(const std::regex& re, const std::string& str, std::string* first_submatch)
{
	std::smatch matches;
	if (!app_regex_partial_match(re, str, matches) || matches.size() < 2) {
		return false;
	}
	if (first_submatch) {
		*first_submatch = matches[1].str();
	}
	return true;
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_partial_match(const std::string& perl_pattern, const std::string& str, std::string* first_submatch)
{
	return app_regex_partial_match(app_regex_re(perl_pattern), str, first_submatch);
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_partial_match(const char* perl_pattern, const std::string& str, std::string* first_submatch)
{
	return app_regex_partial_match(app_regex_re(perl_pattern), str, first_submatch);
}



/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_partial_match(const std::regex& re, const std::string& str, std::vector<std::string*> matches_vector)
{
	std::smatch matches;
	if (!app_regex_partial_match(re, str, matches) || (matches.size() + 1) < matches_vector.size()) {
		return false;
	}

	for (std::size_t i = 1; i < matches_vector.size(); ++i) {
		if (matches_vector[i - 1]) {
			*(matches_vector[i - 1]) = matches.str(i);
		}
	}

	return true;
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_partial_match(const std::string& perl_pattern, const std::string& str, std::vector<std::string*> matches_vector)
{
	return app_regex_partial_match(app_regex_re(perl_pattern), str, matches_vector);
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_partial_match(const char* perl_pattern, const std::string& str, std::vector<std::string*> matches_vector)
{
	return app_regex_partial_match(app_regex_re(perl_pattern), str, matches_vector);
}



/*
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
*/


/// Escape a string to be used inside a regular expression. The result
/// won't contain any special expression characters.
inline std::string app_regex_escape(const std::string& str)
{
	// Based on
	// https://stackoverflow.com/questions/39228912/stdregex-escape-special-characters-for-use-in-regex
	static const std::regex metacharacters(R"([\.\^\$\-\+\(\)\[\]\{\}\|\?\*)");
	return std::regex_replace(str, metacharacters, "\\$&");
}




// ------------------------------------------- Implementation

/*
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
*/



#endif

/// @}
