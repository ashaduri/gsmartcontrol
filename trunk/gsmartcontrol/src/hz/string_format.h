/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_STRING_FORMAT_H
#define HZ_STRING_FORMAT_H

#include "hz_config.h"  // feature macros

#include <string>
#include <cwchar>  // std::wint_t
#include <cstddef>  // std::size_t, std::ptrdiff_t

#include "cstdint.h"  // (u)intmax_t
#include "string_sprintf.h"  // string_sprintf, HZ_FUNC_STRING_SPRINTF_CHECK
#include "stream_cast.h"  // stream_cast<>
#include "local_algo.h"  // returning_binary_search

#include <cassert>
#ifndef ASSERT  // assert() is undefined if NDEBUG is defined.
#	define ASSERT(a) assert(a)
#endif


// sprintf()-like _safe_ formatting to std::string with automatic object->string conversion
// (via << operator).

// Note: These functions use system *printf family of functions.

// Note: If using mingw without __USE_MINGW_ANSI_STDIO,
// you MUST cast long double to double and use %f instead of %Lf;
// also you have to use %I64d instead of %lld for long long int
// and %I64u instead of %llu for unsigned long long int.
// You can use hz::number_to_string() as a workaround.

// FIXME: Check string_sprintf() features through HAVE_STRING_SPRINTF_*
// macros and supply the necessary specifiers / casts.


namespace hz {



namespace internal {
	struct FormatState;
}



// Public API:

inline internal::FormatState string_format(std::string& s, const char* format) HZ_FUNC_STRING_SPRINTF_CHECK(2, 0);

inline internal::FormatState string_format(std::string& s, const std::string& format);




// ---------------------------------- Implementation


namespace internal {


	// -------------------- Printf Format Parsing


	enum string_format_arg_type_t {
		// Conversions: %MT, T is base type to expect, M is (optional) length modifier.

		// Integral:
		// d, i (int); o, u, x, or X (unsigned int).
		// for each of them, modifiers:
			// none: int / unsigned int.
			// hh: signed char / unsigned char (printed as integer)
			// h: short / unsigned short
			// l: long / unsigned long
			// ll: long long / unsigned long long
			// j: intmax_t, uintmax_t
			// z: (s)size_t
			// t: ptrdiff_t

		// Floating point:
		// a, A (double / hex); e, E (double / scientific); f, F (double / normal) g, or G (double / auto).
			// none: double
			// L: long double

		// Other:
		// c (reads int, prints one char).
			// lc (reads wint_t (aka int), prints as wide char (not wchar_t!)).
		// s (const char*).
			// ls (const wchar_t*).
		// p (const void* printed in hex).

		// n (number of chars written so far is put into pointer argument (e.g. int*))
			// none: int*
			// hh: signed char*
			// h: short int*
			// l: long int*
			// ll: long long int*

		// Expected types:
		format_type_schar,
		format_type_uchar,
		format_type_int,
		format_type_uint,
		format_type_short,
		format_type_ushort,
		format_type_long,
		format_type_ulong,
#if !(defined DISABLE_LL_INT && DISABLE_LL_INT)
		format_type_longlong,
#endif
#if !(defined DISABLE_ULL_INT && DISABLE_ULL_INT)
		format_type_ulonglong,
#endif
		format_type_intmax_t,
		format_type_uintmax_t,
		format_type_ssize_t,
		format_type_size_t,
		format_type_ptrdiff_t,

		format_type_double,
		format_type_long_double,

		format_type_char,
		format_type_wchar_t,
		format_type_wint_t,
		format_type_const_charp,
		format_type_const_wchar_tp,
		format_type_const_voidp,

		format_type_intp,
		format_type_scharp,
		format_type_shortp,
		format_type_longp,
#if !(defined DISABLE_LL_INT && DISABLE_LL_INT)
		format_type_longlongp,
#endif

		format_type_unknown  // usually returned on error
	};


	struct conversion_info {
		char s;  // conversion specifier
		char m;  // modifier
		string_format_arg_type_t type;  // type

		static bool has_double(char c)
		{
			return (c == 'h' || c == 'l');
		}
	};


	inline bool operator< (const conversion_info& c1, const conversion_info& c2)
	{
		return (c1.s < c2.s) || (c1.s == c2.s && c1.m < c2.m);
	}

	inline bool operator== (const conversion_info& c1, const conversion_info& c2)
	{
		return (c1.s == c2.s) && (c1.m == c2.m);
	}



	// declarations (in template, to avoid multiple definitions)
	template<typename Dummy>
	struct ConversionTableHolder {
		static const conversion_info conversion_info_table[];
		static const unsigned int conversion_info_table_size;
	};


	// definitions.
	template<typename Dummy> const conversion_info
	ConversionTableHolder<Dummy>::conversion_info_table[] = {
		// Note: This is a sorted array. Double-modifier variants MUST follow
		// the similar single variants.

		// floating point
		{'A', '\0', format_type_double},
		{'A', 'L', format_type_long_double},
		{'E', '\0', format_type_double},
		{'E', 'L', format_type_long_double},
		{'F', '\0', format_type_double},
		{'F', 'L', format_type_long_double},
		{'G', '\0', format_type_double},
		{'G', 'L', format_type_long_double},

		// same expected type as o
		{'X', '\0', format_type_uint},
		{'X', 'h', format_type_ushort},
		{'X', 'h', format_type_uchar},  // hh
		{'X', 'j', format_type_uintmax_t},
		{'X', 'l', format_type_ulong},
#if !(defined DISABLE_ULL_INT && DISABLE_ULL_INT)
		{'X', 'l', format_type_ulonglong},  // ll
#endif
		{'X', 't', format_type_ptrdiff_t},
		{'X', 'z', format_type_size_t},

		// floating point
		{'a', '\0', format_type_double},
		{'a', 'L', format_type_long_double},

		// single character
		{'c', '\0', format_type_char},
		{'c', 'l', format_type_wint_t},

		// signed
		{'d', '\0', format_type_int},
		{'d', 'h', format_type_short},
		{'d', 'h', format_type_schar},  // hh
		{'d', 'j', format_type_intmax_t},
		{'d', 'l', format_type_long},
#if !(defined DISABLE_LL_INT && DISABLE_LL_INT)
		{'d', 'l', format_type_longlong},  // ll
#endif
		{'d', 't', format_type_ptrdiff_t},
		{'d', 'z', format_type_ssize_t},

		// floating point
		{'e', '\0', format_type_double},
		{'e', 'L', format_type_long_double},
		{'f', '\0', format_type_double},
		{'f', 'L', format_type_long_double},
		{'g', '\0', format_type_double},
		{'g', 'L', format_type_long_double},

		// same as d
		{'i', '\0', format_type_int},
		{'i', 'h', format_type_short},
		{'i', 'h', format_type_schar},  // hh
		{'i', 'j', format_type_intmax_t},
		{'i', 'l', format_type_long},
#if !(defined DISABLE_LL_INT && DISABLE_LL_INT)
		{'i', 'l', format_type_longlong},  // ll
#endif
		{'i', 't', format_type_ptrdiff_t},
		{'i', 'z', format_type_ssize_t},

		// info retrieval
		{'n', '\0', format_type_intp},
		{'n', 'h', format_type_shortp},
		{'n', 'h', format_type_scharp},  // hh
		{'n', 'l', format_type_longp},
#if !(defined DISABLE_LL_INT && DISABLE_LL_INT)
		{'n', 'l', format_type_longlongp},  // ll
#endif

		// unsigned
		{'o', '\0', format_type_uint},
		{'o', 'h', format_type_ushort},
		{'o', 'h', format_type_uchar},  // hh
		{'o', 'j', format_type_uintmax_t},
		{'o', 'l', format_type_ulong},
#if !(defined DISABLE_ULL_INT && DISABLE_ULL_INT)
		{'o', 'l', format_type_ulonglong},  // ll
#endif
		{'o', 't', format_type_ptrdiff_t},
		{'o', 'z', format_type_size_t},

		// void* hex address
		{'p', '\0', format_type_const_voidp},

		// strings
		{'s', '\0', format_type_const_charp},
		{'s', 'l', format_type_const_wchar_tp},

		// same expected type as o
		{'u', '\0', format_type_uint},
		{'u', 'h', format_type_ushort},
		{'u', 'h', format_type_uchar},  // hh
		{'u', 'j', format_type_uintmax_t},
		{'u', 'l', format_type_ulong},
#if !(defined DISABLE_ULL_INT && DISABLE_ULL_INT)
		{'u', 'l', format_type_ulonglong},  // ll
#endif
		{'u', 't', format_type_ptrdiff_t},
		{'u', 'z', format_type_size_t},

		// same expected type as o
		{'x', '\0', format_type_uint},
		{'x', 'h', format_type_ushort},
		{'x', 'h', format_type_uchar},  // hh
		{'x', 'j', format_type_uintmax_t},
		{'x', 'l', format_type_ulong},
#if !(defined DISABLE_ULL_INT && DISABLE_ULL_INT)
		{'x', 'l', format_type_ulonglong},  // ll
#endif
		{'x', 't', format_type_ptrdiff_t},
		{'x', 'z', format_type_size_t},
	};



	template<typename Dummy> const unsigned int
	ConversionTableHolder<Dummy>::conversion_info_table_size =
			sizeof(conversion_info_table) / sizeof(conversion_info);


	typedef ConversionTableHolder<void> FormatStaticEnv;  // one (and only) specialization.



	inline const conversion_info* lookup_conversion_either(char c)
	{
		const conversion_info* info = FormatStaticEnv::conversion_info_table;
		const conversion_info* end = info + FormatStaticEnv::conversion_info_table_size;
		while (info != end) {
			if (info->s == c || info->m == c)
				return info;
			++info;
		}

		return NULL;
	}


	inline const conversion_info* lookup_conversion_both(char s, char m, bool dm)
	{
		const conversion_info* info = FormatStaticEnv::conversion_info_table;
		const conversion_info* end = info + FormatStaticEnv::conversion_info_table_size;
// 		while (info != end) {
// 			if (info->s == s && info->m == m) {
// 				if (dm)
// 					++info;  // double variants are always after single variants.
// 				return info;
// 			}
// 			++info;
// 		}
		conversion_info v;
		v.s = s; v.m =m;

		if ((info = hz::returning_binary_search(info, end, v)) != end) {
			if (dm) {
				const conversion_info* info2 = info;
				++info2;
				if (info2 != end && *info2 == *info)
					return info2;

			} else {  // no dm
				if (info != FormatStaticEnv::conversion_info_table) {
					const conversion_info* info2 = info;
					--info2;
					if (*info2 == *info)
						return info2;
				}
			}

			return info;
		}

		return NULL;
	}



	// format is a pointer to a string part, just after %.
	inline string_format_arg_type_t string_printf_get_specifier_type(const char* format)
	{
		char c = *format;

		const conversion_info* info = NULL;
		while (c && !(info = lookup_conversion_either(c))) {
			// nothing to do, just skipping non-conversion and non-modifier chars.
			// note: this is quite error-prone, we take the validity of format for granted
			// (e.g. there could be garbage between % and conv/mod chars).
			c = *(++format);
		}
		if (c == '\0')  // invalid format specifier
			return format_type_unknown;

		// we are in modifier / specifier zone now

		char m = 0;  // modifier
		char s = c;  // specifier
		bool dm = false;  // double-modifier

		// it's a modifier
		if (info->m == c) {
			s = *(++format);  // specifier (or it could be double-modifier)
			m = c;

			if (conversion_info::has_double(m) && s == c) {  // check for ll, hh, etc...
				s = *(++format);  // move to specifier
				dm = true;
			}
		}

		info = lookup_conversion_both(s, m, dm);

		return (info ? info->type : format_type_unknown);
	}




	// advance format to include one % specifier, possibly to the next one
	inline bool string_printf_next_position(const char*& format, string_format_arg_type_t& type)
	{
		if (!format)
			return false;

		bool found = false;

		// Suppose we have a format string of "ABC %ld JKL %e XYZ".
		// The first call to this function should return "ABC %ld JKL ",
		// with the second call returning "%e XYZ".

		// The first pass is to skip to the first %spec.
		// The second pass will skip to the next available %spec (right before it).
		for (unsigned int i = 0; i < 2; ++i) {

			while (*format != '\0') {
				if (*format != '%') {  // skip non-control stuff until the first % or EOL.
					++format;

				} else {  // it's %
					const char* f2 = format;  // lookahead pointer
					if (*(++f2) == '%') {  // %%
						format = ++f2;

					} else {
						found = true;  // doesn't matter which pass
						break;  // we found %spec, position in front of it
					}
				}
			}

			if (found && i == 0) {
				++format;  // skip the % of %spec
				type = string_printf_get_specifier_type(format);
				if (type == format_type_unknown)
					return false;  // invalid %spec
			}
		}

		if (*format == '\0')
			format = 0;  // zero-out, indicates EOL

		return (found || !format);  // found or end of line
	}



	// FormatState holds the formatting state, and implements operator() to format stuff

	struct FormatState {

		std::string& result;  // where to put the output at the end.
		const char* format;  // the printf-like format string
		const char* format_position;  // last position
		bool bad_status;  // if something went wrong.


		FormatState(std::string& s, const char* fmt)
			: result(s), format(fmt), format_position(fmt), bad_status(false)
		{ }


		bool bad()
		{
			return bad_status;
		}



		// Split format string into components, each component
		// containing one %, and possibly surrounding text.
		string_format_arg_type_t move_next(std::string& format_component)
		{
			if (!format_position) {
				bad_status = true;  // extra argument supplied
				return format_type_unknown;
			}

			// search for the next %, advance position pointer, get type.
			const char* oldpos = format_position;
			string_format_arg_type_t type = format_type_unknown;
			if (!string_printf_next_position(format_position, type)) {
				bad_status = true;  // format string error or too many parameters
				return format_type_unknown;
			}
			if (format_position) {
				format_component = std::string(oldpos, format_position - oldpos);
			} else {
				format_component = std::string(oldpos);
			}

			return type;
		}



		// generic variant, prints objects via operator <<
		template<typename T>
		FormatState& operator()(const T& arg)
		{
			string_format_arg_type_t type = format_type_unknown;
			std::string fc;
			if ( (type = this->move_next(fc)) == format_type_unknown)
				return *this;  // error, bad status already set.

			switch(type) {
				case format_type_const_charp:
					result += hz::string_sprintf(fc.c_str(), hz::stream_cast<std::string>(arg).c_str()); break;

				default:
					bad_status = true;  // bad type
					result += fc;  // append the format component to it, so we can see what went wrong.
					break;
			}
			return *this;
		}


		// const char* variant is defined later
		FormatState& operator()(const std::string& arg)
		{
			string_format_arg_type_t type = format_type_unknown;
			std::string fc;
			if ( (type = this->move_next(fc)) == format_type_unknown)
				return *this;

			switch(type) {
				case format_type_const_charp:
					result += hz::string_sprintf(fc.c_str(), arg.c_str()); break;

				default:
					bad_status = true;  // bad type
					result += fc;  // append the format component to it, so we can see what went wrong.
					break;
			}
			return *this;
		}



		// ------------------------------------ Integers


		#define HZ_FORMAT_STATE_OPP_SPEC(T, INTFL) \
		FormatState& operator()(T arg) \
		{ \
			string_format_arg_type_t type = format_type_unknown; \
			std::string fc; \
			if ( (type = this->move_next(fc)) == format_type_unknown) \
				return *this; \
			this->format_ ## INTFL(fc.c_str(), type, arg); \
			return *this; \
		}

		template<typename T>
		void format_int(const char* fc, string_format_arg_type_t type, T arg)
		{
			// cast T into any type it can handle
			switch(type) {
				case format_type_schar: result += hz::string_sprintf(fc, static_cast<signed char>(arg)); break;
				case format_type_uchar: result += hz::string_sprintf(fc, static_cast<unsigned char>(arg)); break;
				case format_type_int: result += hz::string_sprintf(fc, static_cast<int>(arg)); break;
				case format_type_uint: result += hz::string_sprintf(fc, static_cast<unsigned int>(arg)); break;
				case format_type_short: result += hz::string_sprintf(fc, static_cast<short int>(arg)); break;
				case format_type_ushort: result += hz::string_sprintf(fc, static_cast<unsigned short int>(arg)); break;
				case format_type_long: result += hz::string_sprintf(fc, static_cast<long int>(arg)); break;
				case format_type_ulong: result += hz::string_sprintf(fc, static_cast<unsigned long int>(arg)); break;
#if !(defined DISABLE_LL_INT && DISABLE_LL_INT)
				case format_type_longlong: result += hz::string_sprintf(fc, static_cast<long long int>(arg)); break;
#endif
#if !(defined DISABLE_ULL_INT && DISABLE_ULL_INT)
				case format_type_ulonglong: result += hz::string_sprintf(fc, static_cast<unsigned long long int>(arg)); break;
#endif
				case format_type_intmax_t: result += hz::string_sprintf(fc, static_cast<intmax_t>(arg)); break;
				case format_type_uintmax_t: result += hz::string_sprintf(fc, static_cast<uintmax_t>(arg)); break;
				// note: ssize_t is non-standard (from unistd.h), so we transform size_t to signed.
				case format_type_ssize_t: result += hz::string_sprintf(fc, static_cast<typename hz::type_make_signed<std::size_t>::type>(arg)); break;
				case format_type_size_t: result += hz::string_sprintf(fc, static_cast<std::size_t>(arg)); break;
				case format_type_ptrdiff_t: result += hz::string_sprintf(fc, static_cast<std::ptrdiff_t>(arg)); break;

				// ints can be passed in double specifiers
				case format_type_double: result += hz::string_sprintf(fc, static_cast<double>(arg)); break;
				case format_type_long_double: result += hz::string_sprintf(fc, static_cast<long double>(arg)); break;

				case format_type_char: result += hz::string_sprintf(fc, static_cast<char>(arg)); break;
				case format_type_wchar_t: result += hz::string_sprintf(fc, static_cast<wchar_t>(arg)); break;
				case format_type_wint_t: result += hz::string_sprintf(fc, static_cast<std::wint_t>(arg)); break;

				default:
					bad_status = true;  // bad type
					result += fc;  // append the format component to it, so we can see what went wrong.
					break;
			}
		}

		// all these types can be used in integer specifiers.
		HZ_FORMAT_STATE_OPP_SPEC(bool, int)

		HZ_FORMAT_STATE_OPP_SPEC(char, int)
		HZ_FORMAT_STATE_OPP_SPEC(signed char, int)
		HZ_FORMAT_STATE_OPP_SPEC(unsigned char, int)
		HZ_FORMAT_STATE_OPP_SPEC(wchar_t, int)

		HZ_FORMAT_STATE_OPP_SPEC(short int, int)
		HZ_FORMAT_STATE_OPP_SPEC(unsigned short int, int)
		HZ_FORMAT_STATE_OPP_SPEC(int, int)
		HZ_FORMAT_STATE_OPP_SPEC(unsigned int, int)
		HZ_FORMAT_STATE_OPP_SPEC(long int, int)
		HZ_FORMAT_STATE_OPP_SPEC(unsigned long int, int)
		HZ_FORMAT_STATE_OPP_SPEC(long long int, int)
		HZ_FORMAT_STATE_OPP_SPEC(unsigned long long int, int)



		template<typename T>
		void format_float(const char* fc, string_format_arg_type_t type, T arg)
		{
			// cast T into any type it can handle
			switch(type) {
				case format_type_double: result += hz::string_sprintf(fc, static_cast<double>(arg)); break;
				case format_type_long_double: result += hz::string_sprintf(fc, static_cast<long double>(arg)); break;

				default:
					bad_status = true;  // bad type
					result += fc;  // append the format component to it, so we can see what went wrong.
					break;
			}
		}

		HZ_FORMAT_STATE_OPP_SPEC(float, float)
		HZ_FORMAT_STATE_OPP_SPEC(double, float)
		HZ_FORMAT_STATE_OPP_SPEC(long double, float)


		#undef HZ_FORMAT_STATE_OPP_SPEC



		#define HZ_FORMAT_STATE_OPP_COMMON_SPEC(T, FT) \
		FormatState& operator()(T arg) \
		{ \
			string_format_arg_type_t type = format_type_unknown; \
			std::string fc; \
			if ( (type = this->move_next(fc)) == format_type_unknown) \
				return *this; \
			switch(type) { \
				case FT: \
					result += hz::string_sprintf(fc.c_str(), arg); break; \
				default: \
					bad_status = true; \
					result += fc; \
					break; \
			} \
			return *this; \
		}


		HZ_FORMAT_STATE_OPP_COMMON_SPEC(const char*, format_type_const_charp: case format_type_const_voidp)
		HZ_FORMAT_STATE_OPP_COMMON_SPEC(const wchar_t*, format_type_const_wchar_tp: case format_type_const_voidp)
		HZ_FORMAT_STATE_OPP_COMMON_SPEC(const void*, format_type_const_voidp)

		// None: These won't actually return the correct data (because of component-sprintf),
		// but we still verify them for type-correctness.
		HZ_FORMAT_STATE_OPP_COMMON_SPEC(int*, format_type_intp)
		HZ_FORMAT_STATE_OPP_COMMON_SPEC(signed char*, format_type_scharp)
		HZ_FORMAT_STATE_OPP_COMMON_SPEC(short int*, format_type_shortp)
		HZ_FORMAT_STATE_OPP_COMMON_SPEC(long int*, format_type_longp)
#if !(defined DISABLE_LL_INT && DISABLE_LL_INT)
		HZ_FORMAT_STATE_OPP_COMMON_SPEC(long long int*, format_type_longlongp)
#endif


		#undef HZ_FORMAT_STATE_OPP_COMMON_SPEC


	};



}  // ns internal





// ------------------------------ Main functions


inline internal::FormatState string_format(std::string& s, const char* format)
{
	return internal::FormatState(s, format);
}

inline internal::FormatState string_format(std::string& s, const std::string& format)
{
	return internal::FormatState(s, format.c_str());
}





}  // ns




#endif
