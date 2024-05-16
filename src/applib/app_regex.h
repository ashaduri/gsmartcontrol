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

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include <regex>

#include "hz/debug.h"
#include "hz/string_algo.h"


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





/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_full_match(const std::regex& re, const std::string& str)
{
	return std::regex_match(str, re);
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_full_match(const std::string& perl_pattern, const std::string& str)
{
	return app_regex_full_match(app_regex_re(perl_pattern), str);
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_full_match(const char* perl_pattern, const std::string& str)
{
	return app_regex_full_match(app_regex_re(perl_pattern), str);
}



/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_full_match(const std::regex& re, const std::string& str, std::smatch& matches)
{
	return std::regex_match(str, matches, re);
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_full_match(const std::string& perl_pattern, const std::string& str, std::smatch& matches)
{
	return app_regex_full_match(app_regex_re(perl_pattern), str, matches);
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_full_match(const char* perl_pattern, const std::string& str, std::smatch& matches)
{
	return app_regex_full_match(app_regex_re(perl_pattern), str, matches);
}



/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_full_match(const std::regex& re, const std::string& str, std::string* first_submatch)
{
	std::smatch matches;
	if (!app_regex_full_match(re, str, matches) || matches.size() < 2) {
		return false;
	}
	if (first_submatch) {
		*first_submatch = matches[1].str();
	}
	return true;
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_full_match(const std::string& perl_pattern, const std::string& str, std::string* first_submatch)
{
	return app_regex_full_match(app_regex_re(perl_pattern), str, first_submatch);
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_full_match(const char* perl_pattern, const std::string& str, std::string* first_submatch)
{
	return app_regex_full_match(app_regex_re(perl_pattern), str, first_submatch);
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_full_match(const std::regex& re, const std::string& str, std::vector<std::string*> matches_vector)
{
	std::smatch matches;
	if (!app_regex_full_match(re, str, matches) || (matches.size() + 1) < matches_vector.size()) {
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
inline bool app_regex_full_match(const std::string& perl_pattern, const std::string& str, std::vector<std::string*> matches_vector)
{
	return app_regex_full_match(app_regex_re(perl_pattern), str, matches_vector);
}


/// Partially match a string against a regular expression.
/// \return true if a match was found.
inline bool app_regex_full_match(const char* perl_pattern, const std::string& str, std::vector<std::string*> matches_vector)
{
	return app_regex_full_match(app_regex_re(perl_pattern), str, matches_vector);
}






/// Replace every occurrence of pattern with replacement string in \c subject.
/// \return number of replacements made.
inline void app_regex_replace(const std::regex& re, const std::string& replacement, std::string& subject)
{
	subject = std::regex_replace(subject, re, replacement);
}


/// Replace every occurrence of pattern with replacement string in \c subject.
/// The pattern is in "/pattern/modifiers" format.
/// \return number of replacements made.
inline void app_regex_replace(const std::string& perl_pattern, const std::string& replacement, std::string& subject)
{
	app_regex_replace(app_regex_re(perl_pattern), replacement, subject);
}


/// Replace every occurrence of pattern with replacement string in \c subject.
/// The pattern is in "/pattern/modifiers" format.
/// \return number of replacements made.
inline void app_regex_replace(const char* perl_pattern, const std::string& replacement, std::string& subject)
{
	app_regex_replace(app_regex_re(perl_pattern), replacement, subject);
}




/// Escape a string to be used inside a regular expression. The result
/// won't contain any special expression characters.
inline std::string app_regex_escape(const std::string& str)
{
	// Based on
	// https://stackoverflow.com/questions/39228912/stdregex-escape-special-characters-for-use-in-regex
	static const std::map<std::string, std::string> replacements {
			{"\\", "\\\\"},
			{"^", "\\^"},
			{"$", "\\$"},
//			{"-", "\\-"},
			{"|", "\\|"},
			{".", "\\."},
			{"?", "\\?"},
			{"*", "\\*"},
			{"+", "\\+"},
			{"(", "\\("},
			{")", "\\)"},
			{"[", "\\["},
			{"]", "\\]"},
			{"{", "\\{"},
			{"}", "\\}"},
	};
	return hz::string_replace_array_copy(str, replacements);
}





#endif

/// @}
