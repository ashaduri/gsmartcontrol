/**************************************************************************
 Copyright:
      (C) 1989, 1993, 1994  The Regents of the University of California
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_bsd-ucb.txt file
***************************************************************************/

#ifndef HZ_STRING_WCMATCH_H
#define HZ_STRING_WCMATCH_H

#include "hz_config.h"  // feature macros


// Parts of this file are derived from Sudo's fnmatch().

// The original copyright header follows:
/*
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Guido van Rossum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */



#include <string>
#include <cctype>  // std::tolower
#include <cstring>  // std::strchr



namespace hz {


namespace internal {

	// internal constants. HZ_ prefix is needed to avoid conflicts with libc version.
	const int HZ_FNM_NOMATCH = 1; /* Match failed. */

	const int HZ_FNM_NOESCAPE = 0x01; /* Disable backslash escaping. */
	const int HZ_FNM_PATHNAME = 0x02; /* Slash must be matched by slash. */
	const int HZ_FNM_PERIOD = 0x04; /* Period must be matched by period. */
	const int HZ_FNM_LEADING_DIR = 0x08; /* Ignore /<tail> after Imatch. */
	const int HZ_FNM_CASEFOLD = 0x10; /* Case insensitive search. */
	const int HZ_FNM_IGNORECASE = HZ_FNM_CASEFOLD;
	const int HZ_FNM_FILE_NAME= HZ_FNM_PATHNAME;



	#define HZ_WC_ISSET(t, f) ((t) & (f))


	enum wc_range_status_t {
		HZ_RANGE_MATCH = 1,
		HZ_RANGE_NOMATCH = 0,
		HZ_RANGE_ERROR = -1
	};


	// helper function
	inline wc_range_status_t wc_rangematch(const char* pattern, char test, int flags, const char** newp)
	{
		int negate = 0, ok = 0;
		char c = 0, c2 = 0;

		/*
		* A bracket expression starting with an unquoted circumflex
		* character produces unspecified results (IEEE 1003.2-1992,
		* 3.13.2).  This implementation treats it like '!', for
		* consistency with the regular expression syntax.
		* J.T. Conklin (conklin@ngai.kaleida.com)
		*/
		if ((negate = (*pattern == '!' || *pattern == '^')))
			++pattern;

		if (HZ_WC_ISSET(flags, HZ_FNM_CASEFOLD))
			test = static_cast<char>(std::tolower(static_cast<unsigned char>(test)));

		/*
		* A right bracket shall lose its special meaning and represent
		* itself in a bracket expression if it occurs first in the list.
		* -- POSIX.2 2.8.3.2
		*/
		c = *pattern++;
		do {
			if (c == '\\' && !HZ_WC_ISSET(flags, HZ_FNM_NOESCAPE))
				c = *pattern++;

			if (c == '\0')
				return (HZ_RANGE_ERROR);

			if (c == '/' && HZ_WC_ISSET(flags, HZ_FNM_PATHNAME))
				return (HZ_RANGE_NOMATCH);

			if (HZ_WC_ISSET(flags, HZ_FNM_CASEFOLD))
				c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

			if (*pattern == '-' && (c2 = *(pattern+1)) != '\0' && c2 != ']') {
				pattern += 2;

				if (c2 == '\\' && !HZ_WC_ISSET(flags, HZ_FNM_NOESCAPE))
					c2 = *pattern++;

				if (c2 == '\0')
					return (HZ_RANGE_ERROR);

				if (HZ_WC_ISSET(flags, HZ_FNM_CASEFOLD))
					c2 = static_cast<char>(std::tolower((unsigned char)c2));

				if (c <= test && test <= c2)
					ok = 1;

			} else if (c == test) {
				ok = 1;
			}
		} while ((c = *pattern++) != ']');

		*newp = pattern;
		return (ok == negate ? HZ_RANGE_NOMATCH : HZ_RANGE_MATCH);
	}




	/*
	* Function fnmatch() as specified in POSIX 1003.2-1992, section B.6.
	* Compares a filename or pathname to a pattern.
	*/
	inline int wc_fnmatch(const char* pattern, const char* str, int flags)
	{
		const char* strstart = 0;
		char c = 0, test = 0;

		for (strstart = str;;) {
			switch (c = *pattern++) {
				case '\0':
					if (HZ_WC_ISSET(flags, HZ_FNM_LEADING_DIR) && *str == '/')
						return (0);
					return (*str == '\0' ? 0 : HZ_FNM_NOMATCH);

				case '?':
					if (*str == '\0')
						return (HZ_FNM_NOMATCH);

					if (*str == '/' && HZ_WC_ISSET(flags, HZ_FNM_PATHNAME))
						return (HZ_FNM_NOMATCH);

					if (*str == '.' && HZ_WC_ISSET(flags, HZ_FNM_PERIOD) &&
							(str == strstart || (HZ_WC_ISSET(flags, HZ_FNM_PATHNAME) && *(str - 1) == '/'))) {
						return (HZ_FNM_NOMATCH);
					}

					++str;
					break;

				case '*':
					c = *pattern;
					/* Collapse multiple stars. */
					while (c == '*')
						c = *++pattern;

					if (*str == '.' && HZ_WC_ISSET(flags, HZ_FNM_PERIOD) &&
							(str == strstart || (HZ_WC_ISSET(flags, HZ_FNM_PATHNAME) && *(str - 1) == '/')))
						return (HZ_FNM_NOMATCH);

					/* Optimize for pattern with * at end or before /. */
					if (c == '\0') {
						if (HZ_WC_ISSET(flags, HZ_FNM_PATHNAME)) {
							return (HZ_WC_ISSET(flags, HZ_FNM_LEADING_DIR) || std::strchr(str, '/') == NULL ?
								0 : HZ_FNM_NOMATCH);
						} else {
							return (0);
						}

					} else if (c == '/' && HZ_WC_ISSET(flags, HZ_FNM_PATHNAME)) {
						if ((str = std::strchr(str, '/')) == NULL)
							return (HZ_FNM_NOMATCH);
						break;
					}

					/* General case, use recursion. */
					while ((test = *str) != '\0') {
						if (!wc_fnmatch(pattern, str, flags & ~HZ_FNM_PERIOD))
							return (0);

						if (test == '/' && HZ_WC_ISSET(flags, HZ_FNM_PATHNAME))
							break;
						++str;
					}
					return (HZ_FNM_NOMATCH);

				case '[':
				{
					if (*str == '\0')
						return (HZ_FNM_NOMATCH);

					if (*str == '/' && HZ_WC_ISSET(flags, HZ_FNM_PATHNAME))
						return (HZ_FNM_NOMATCH);

					if (*str == '.' && HZ_WC_ISSET(flags, HZ_FNM_PERIOD) &&
							(str == strstart || (HZ_WC_ISSET(flags, HZ_FNM_PATHNAME) && *(str - 1) == '/'))) {
						return (HZ_FNM_NOMATCH);
					}

					const char* newp = 0;
					switch (wc_rangematch(pattern, *str, flags, &newp)) {
						case HZ_RANGE_ERROR:
							/* not a good range, treat as normal text */
							goto normal;

						case HZ_RANGE_MATCH:
							pattern = newp;
							break;

						case HZ_RANGE_NOMATCH:
							return (HZ_FNM_NOMATCH);
					}

					++str;
					break;
				}

				case '\\':
					if (!HZ_WC_ISSET(flags, HZ_FNM_NOESCAPE)) {
						if ((c = *pattern++) == '\0') {
							c = '\\';
							--pattern;
						}
					}
					/* FALLTHROUGH */

				default:
				normal:  // goto label
					if (c != *str && !(HZ_WC_ISSET(flags, HZ_FNM_CASEFOLD) &&
							(std::tolower(static_cast<unsigned char>(c)) == std::tolower(static_cast<unsigned char>(*str)))))
						return (HZ_FNM_NOMATCH);
					++str;
					break;
			}
		}
		/* NOTREACHED */
		return (HZ_FNM_NOMATCH);  // just in case
	}


	#undef HZ_WC_ISSET



	// return true if pattern has any glob chars in it
	inline bool wc_fnmatch_test(const char* pattern)
	{
		bool in_bracket = false;

		while (*pattern) {
			switch (*pattern) {
				case '?':
				case '*':
					return true;

				case '\\':
					if (*pattern++ == '\0')
						return false;
					break;

				case '[':	  // a string containing '[' is a glob only if it has a matching ']'
					in_bracket = true;
					break;

				case ']':
					if (in_bracket)
						return true;
					break;
			}
			++pattern;
		}

		return false;
	}



}  // ns internal




// -------------- std::string-style API


// Check whether str matches pattern, which is a shell wildcard.
// On non-match or failure false is returned; otherwise, true.
inline bool string_wcmatch(const std::string& pattern, const std::string& str)
{
	return (hz::internal::wc_fnmatch(pattern.c_str(), str.c_str(),
			internal::HZ_FNM_PATHNAME | internal::HZ_FNM_PERIOD) == 0);
}


// Test whether pattern contains any glob characters in it.
inline bool string_is_wc_pattern(const std::string& pattern)
{
	return static_cast<bool>(hz::internal::wc_fnmatch_test(pattern.c_str()));
}




}  // ns hz


#endif
