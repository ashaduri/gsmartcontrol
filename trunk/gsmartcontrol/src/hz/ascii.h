/**************************************************************************
 Copyright:
      (C) 1992, 1993  The Regents of the University of California
 License: See LICENSE_bsd-ucb.txt file
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author The Regents of the University of California
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_ASCII_H
#define HZ_ASCII_H

#include "hz_config.h"  // feature macros


/// \file
/// Part of this file (specifically, ascii_strtoi() implementation)
/// is derived from FreeBSD's strtol() and friends.

// The original FreeBSD copyright header follows:
/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
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
// Taken from:
// __FBSDID("$FreeBSD: src/lib/libc/stdlib/strtol.c,v 1.20 2007/01/09 00:28:10 imp Exp $");



#include <limits>  // std::numeric_limits
#include <cerrno>  // errno (not std::errno, it may be a macro)
#include <clocale>  // std::localeconv, std::lconv
#include <cstring>  // std::strlen, std::memcmp, std::memcpy
#include <cstddef>  // std::size_t, std::ptrdiff_t
#include <cstdlib>  // for stdlib.h, std::strtod
#include <cctype>  // std::isspace

#if !(defined DISABLE_STRTOF && DISABLE_STRTOF) \
		|| !(defined DISABLE_STRTOLD && DISABLE_STRTOLD)
	#include <stdlib.h>  // strtof, strtold (C99, not in C++98)
#endif

#include <cassert>
#ifndef ASSERT  // assert() is undefined if NDEBUG is defined.
	#define ASSERT(a) assert(a)
#endif

#include "type_properties.h"  // type_is_*, type_make_unsigned

// Disable silly VS warnings
#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4800)  // 'int' : forcing value to bool 'true' or 'false' (performance warning)
	#pragma warning(disable: 4804)  // unsafe use of type 'bool' in operation
	#pragma warning(disable: 4146)  // unary minus operator applied to unsigned type, result still unsigned
	#pragma warning(disable: 6328)  // 'const char' passed as parameter '1' when 'unsigned char' is required in call to 'isspace'
#endif

// Locale-independent functions for ascii manipulation.



namespace hz {



/// Implementation of std::isspace().
/// This function always behaves like the standard function
/// does in Classic locale (regardless of current locale).
inline bool ascii_isspace(char c)
{
	return (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v');
}



/// Implementation of std::strtoX() functions for every native integral type:
/// char (numerical value), wchar_t (numerical value),
/// int, short, long, long long, plus their signed/unsigned variants.
/// This function always behaves like the standard functions do
/// in Classic locale.
template<typename T> inline
T ascii_strtoi(const char* nptr, char** endptr, int base)
{
	typedef typename hz::type_make_unsigned<T>::type unsigned_type;
	typedef typename hz::type_make_signed<T>::type signed_type;

	const char* s = nptr;
	unsigned_type acc = 0;
	unsigned_type cutoff = 0;
	signed_type cutlim = 0;
	int any = 0;  // (-1, 0, 1)
	char c = 0;
	bool neg = false;

	// skip whitespace and pick up leading +/- sign if any.
	do {
		c = *s++;
	} while (ascii_isspace(c));

	if (c == '-') {
		neg = true;
		c = *s++;
	} else {
		if (c == '+')
			c = *s++;
	}

	// This check is actually not present in original version.
	// So, in original, -1 successfully passes as an unsigned integer.
	// This is obviously wrong - if 32768 causes int16_t overflow, so
	// should -1 cause uint16_t underflow.
	if (neg && !std::numeric_limits<T>::is_signed) {  // disallow negative unsigned integers
		errno = ERANGE;
		if (endptr != 0)
			*endptr = const_cast<char*>(--s);  // where '-' was encountered
		return std::numeric_limits<T>::min();  // aka 0
	}

	// If base is 0, allow 0x for hex and 0 for octal, else
	// assume decimal; if base is already 16, allow 0x.
	if ( (base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X') &&
			((s[1] >= '0' && s[1] <= '9') || (s[1] >= 'A' && s[1] <= 'F') || (s[1] >= 'a' && s[1] <= 'f')) ) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = (c == '0' ? 8 : 10);

	if (base < 2 || base > 36) {
		errno = EINVAL;
		if (endptr != 0)
			*endptr = const_cast<char*>(nptr);
		return 0;
	}

	acc = 0;
	any = 0;

	// Compute the cutoff value between legal numbers and illegal
	// numbers.  That is the largest legal value, divided by the
	// base.  An input number that is greater than this value, if
	// followed by a legal input character, is too big.  One that
	// is equal to this value may be valid or not; the limit
	// between valid and invalid numbers is then based on the last
	// digit.  For instance, if the range for longs is
	// [-2147483648..2147483647] and the input base is 10,
	// cutoff will be set to 214748364 and cutlim to either
	// 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
	// a value > 214748364, or equal but the next digit is > 7 (or 8),
	// the number is too big, and we will return a range error.
	//
	// Set 'any' if any `digits' consumed; make it negative to indicate
	// overflow.

	if (std::numeric_limits<T>::is_signed) {
#ifdef __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wint-in-bool-context"
	#pragma GCC diagnostic ignored "-Woverflow"
#endif
		cutoff = static_cast<unsigned_type>( neg ?
				( static_cast<unsigned_type>(-(std::numeric_limits<T>::min() + std::numeric_limits<T>::max()))
				+ std::numeric_limits<T>::max() ) : std::numeric_limits<T>::max() );
#ifdef __GNUC__
	#pragma GCC diagnostic pop
#endif
		cutlim = static_cast<signed_type>(cutoff % base);
		cutoff = static_cast<unsigned_type>(cutoff / base);
	} else {
		cutoff = static_cast<unsigned_type>(std::numeric_limits<T>::max() / base);
		cutlim = static_cast<signed_type>(std::numeric_limits<T>::max() % base);
	}

	// Assumes that the upper and lower case
	// alphabets and digits are each contiguous.
	for ( ; ; c = *s++) {
		if (c >= '0' && c <= '9') {
			c = static_cast<char>(c - '0');
		} else if (c >= 'A' && c <= 'Z') {
			c = static_cast<char>(c - 'A' + 10);
		} else if (c >= 'a' && c <= 'z') {
			c = static_cast<char>(c - 'a' + 10);
		} else {
			break;
		}
		if (c >= base) {
			break;
		}
		if ((any < 0) || acc > cutoff || (acc == cutoff
				&& static_cast<signed_type>(c) > cutlim)) {
			any = -1;
		} else {
			any = 1;
			acc = static_cast<unsigned_type>((acc * base) + c);
		}
	}

	if (any < 0) {
		if (std::numeric_limits<T>::is_signed) {
			acc = neg ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
		} else {
			acc = std::numeric_limits<T>::max();
		}
		errno = ERANGE;

	} else if (!any) {
		errno = EINVAL;

	} else if (neg) {
		acc = static_cast<unsigned_type>(-acc);
	}

	if (endptr != 0)
		*endptr = const_cast<char*>(any ? (s - 1) : nptr);

	return acc;
}




namespace internal {

	// proxy functions for system strtod family.

	/// We use struct because the errors are easier on the user this way.
	template<typename T>
	struct ascii_strtof_impl { };


	/// Specialization
	template<>
	struct ascii_strtof_impl<double> {
		static double func(const char* nptr, char** endptr)
		{
			return std::strtod(nptr, endptr);
		}
	};


	// Note: strtold() and strtof() are C99 / _XOPEN_SOURCE >= 600.

	/// Specialization
	template<>
	struct ascii_strtof_impl<float> {
		static float func(const char* nptr, char** endptr)
		{
		#if !(defined DISABLE_STRTOF && DISABLE_STRTOF)
			return strtof(nptr, endptr);
		#else
			// emulate via strtod().
			double val = std::strtod(nptr, endptr);

			// check the overflow condition. note: min() is smallest positive number, not negative huge.
			if (val > static_cast<double>(std::numeric_limits<float>::max())) {
				errno = ERANGE;
				// infinity is defined as builtin hugeval in limits. HUGE_VALF seems to be C99 only.
				return std::numeric_limits<float>::infinity();
			}

			// negative overflow
			if (val < -static_cast<double>(std::numeric_limits<float>::max())) {
				errno = ERANGE;
				return -std::numeric_limits<float>::infinity();
			}

			// NOTE: We don't check for the underflow. The standards have
			// a mixed definition of it (and what value should be returned), and
			// I'm not sure if a flag should be raised at all.
			// If anyone knows how to do it portably and reliably, please tell me.

			return float(val);
#endif


		}
	};


	/// Specialization
	template<>
	struct ascii_strtof_impl<long double> {
		static long double func(const char* nptr, char** endptr)
		{
		#if !(defined DISABLE_STRTOLD && DISABLE_STRTOLD)
			return strtold(nptr, endptr);
		#else
			// This leads to loss of precision, but what else can we do?

			// nans and infinities are preserved on implicit conversion,
			// so no need to do anything.
			return std::strtod(nptr, endptr);
		#endif
		}
	};


}



// POSIX / C99 behaviour of strtod() / strtof() / strtold():
// 1. optional leading whitespace (std::isspace()).
// 2. optional + or - signs.
// 3. one of the following:
// 	a) decimal number:
//		non-empty sequence of decimal digits, optionally containing radix (decimal
//		point, locale-dependent). optional decimal exponent: e|E[+|-]decimal_digits.
//	b) hexadecimal number:
//		0x or 0X, then non-empty sequence of hex digits (possibly with radix),
//		optional binary exponent: p|P[+|-]decimal_digits. C90 (but not C99) requires
//		that either radix or exponent must be present.
//	c) infinity:
//		"inf" or "infinity", case-insensitive.
//	d) nan:
//		"nan", case-insensitive. optionally followed by "(", sequence of chars, ")".
// Returns: converted value.
// if endptr is not 0, it's set to point to the first character of nptr on which
// the parser stopped.
// if no conversion is performed, it returns 0, sets *endptr to nptr and errno is
// set to EINVAL (setting errno is optional).
// on overflow, +/-HUGE_VAL[L|F] is returned, and errno is set to ERANGE.
// on underflow, 0 is returned and errno is set to ERANGE.

/// Implementation of strtod/strtof/strtold for any floating-point type.
/// This function always behaves like the standard functions do
/// in Classic locale.
template<typename T> inline
T ascii_strtof(const char* nptr, char** endptr)
{
	// Basic checks. Detect early, detect often.
	if (!nptr || nptr[0] == '\0') {
		errno = EINVAL;  // this is optional, but let's do it anyway.
		if (endptr)
			*endptr = const_cast<char*>(nptr);
		return 0;
	}

	// Only decimal point and spaces are locale-dependent, convert them
	// to a format strtod() would understand.

	// Skip leading ascii spaces, so that strtod doesn't have to recognize them.
	const char* wnptr = nptr;
	while (ascii_isspace(*wnptr))
		++wnptr;

	// If after classic whitespaces there are locale whitespaces, error out.
	// Without this, strtod() would ignore them, which is not ok (they shouldn't be here).
	if (std::isspace(*wnptr)) {
		errno = EINVAL;  // this is optional, but let's do it anyway.
		if (endptr)
			*endptr = const_cast<char*>(wnptr);
		return 0;
	}

	std::lconv* locdata = std::localeconv();
	ASSERT(locdata);
	const char* radix = locdata->decimal_point;
	ASSERT(radix);
	std::size_t radix_len = (radix ? std::strlen(radix) : 0U);
	ASSERT(radix_len);

	// Check if the locale resembles Classic.
	if (radix[0] == '.' && radix[1] == '\0') {
		// nothing to convert, just forward to the system function.
		T val = internal::ascii_strtof_impl<T>::func(wnptr, endptr);
		if (endptr && *endptr == wnptr)  // re-set endptr to nptr if they should be equal
			*endptr = const_cast<char*>(nptr);
		return val;
	}

	// If, e.g., the source string is "12,34" and "," is locale radix,
	// we need to stop right before it, or else strtod() will return 12.34
	// instead of 12. (well, unless radix is space, which is really screwed up anyway).
	std::size_t wnptr_len = std::strlen(wnptr);
	const char* end_pos = wnptr;  // right after the number is over.
	for (std::size_t i = 0; i < wnptr_len + 1 - radix_len; ++i) {
		if (*end_pos == '\0' || std::memcmp(end_pos, radix, radix_len) == 0)
			break;
		++end_pos;
	}

	// No real format checks here, but I don't think they are needed
	// anyway. We're just replacing one char with another, and unless
	// the locale has really bad conflicts with it, everything should be fine.
	const char* radix_pos = wnptr;
	while (!(radix_pos == end_pos || *radix_pos == '.')) {
		++radix_pos;
	}


	if (*radix_pos == '.') {  // classic radix found
		// even if radix_len is 1, we still have to copy the string, so no special case there.
		char* copy = new char[end_pos - wnptr + radix_len];  // \0 will take a byte of old '.'.

		std::memcpy(copy, wnptr, radix_pos - wnptr);  // copy up until '.'.
		char* cpos = copy + (radix_pos - wnptr);
		std::memcpy(cpos, radix, radix_len);  // copy the locale radix
		cpos += radix_len;
		std::memcpy(cpos, radix_pos + 1, end_pos - radix_pos - 1);
		cpos += end_pos - radix_pos - 1;
		*cpos = '\0';

		char* cendptr = 0;
		T val = internal::ascii_strtof_impl<T>::func(copy, &cendptr);

		// translate cendptr to *endptr
		if (endptr) {
			if (!cendptr || *cendptr == '\0') {
				*endptr = const_cast<char*>(end_pos);  // location of \0 or first locale radix in original string

			} else if (cendptr) {
				if (cendptr == copy) {
					*endptr = const_cast<char*>(nptr);
				} else {
					std::ptrdiff_t coffset = (cendptr - copy);
					*endptr = const_cast<char*>(wnptr) + coffset - ((coffset > (radix_pos - wnptr)) ? (radix_len - 1) : 0);
				}
			}
		}

		delete[] copy;

		return val;
	}


	if (*end_pos != '\0') {  // locale radix found, use a part of nptr before that.
		char* copy = new char[end_pos - wnptr + 1];
		std::memcpy(copy, wnptr, end_pos - wnptr);
		*(copy + (end_pos - wnptr)) = '\0';

		char* cendptr = 0;
		T val = internal::ascii_strtof_impl<T>::func(copy, &cendptr);

		// translate cendptr to *endptr
		if (endptr) {
			if (!cendptr || *cendptr == '\0') {
				*endptr = const_cast<char*>(end_pos);  // location of \0 or first locale radix in original string

			} else if (cendptr) {
				if (cendptr == copy) {
					*endptr = const_cast<char*>(nptr);
				} else {
					*endptr = const_cast<char*>(wnptr) + (cendptr - copy);
				}
			}
		}

		delete[] copy;

		return val;
	}


	// no classic radix, no locale radix.
	// nothing to convert, just forward to the system function.
	T val = internal::ascii_strtof_impl<T>::func(wnptr, endptr);
	if (endptr && *endptr == wnptr)  // re-set endptr to nptr if they should be equal
		*endptr = const_cast<char*>(nptr);

	return val;
}



namespace {

	/// Helper struct to choose the correct implementation
	template<bool IsInt, bool IsFloat>
	struct ascii_strton_impl { };

	/// Specialization
	template<bool IsInt>
	struct ascii_strton_impl<IsInt, true> {
		template<typename T> inline
		static T func(const char* nptr, char** endptr, int base = 0)
		{
			return hz::ascii_strtof<T>(nptr, endptr);
		}
	};

	/// Specialization
	template<bool IsFloat>
	struct ascii_strton_impl<true, IsFloat> {
		template<typename T> inline
		static T func(const char* nptr, char** endptr, int base = 0)
		{
			return hz::ascii_strtoi<T>(nptr, endptr, base);
		}
	};

}



/// Create any number (integral or floating point) from a string.
/// Note: Parameter "base" is ignored for floating point types.
template<typename T> inline
T ascii_strton(const char* nptr, char** endptr, int base = 0)
{
	return ascii_strton_impl<hz::type_is_integral<T>::value,
			hz::type_is_floating_point<T>::value>::template func<T>(nptr, endptr, base);
}




}  // ns


#ifdef _MSC_VER
	#pragma warning(pop)
#endif


#endif

/// @}
