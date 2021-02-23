/**************************************************************************
 Copyright:
      (C) 2000 - 2010  Kevlin Henney
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_boost_1_0.txt file
***************************************************************************/

#ifndef HZ_LEXICAL_CAST_H
#define HZ_LEXICAL_CAST_H

#include <cstddef>  // std::size_t
#include <string>
#include <sstream>
#include <limits>  // std::numeric_limits
#include <type_traits>

#include "bad_cast_exception.h"


/**
\file
stream_cast library, based on boost::lexical_cast.

Convert any type supporting "operator<<" to any other type
supporting "operator>>".

Original notes and copyright info follow:

// what:  lexical_cast custom keyword cast
// who:   contributed by Kevlin Henney,
//        enhanced with contributions from Terje Slettebø,
//        with additional fixes and suggestions from Gennaro Prota,
//        Beman Dawes, Dave Abrahams, Daryle Walker, Peter Dimov,
//        and other Boosters
// when:  November 2000, March 2003, June 2005

// Copyright Kevlin Henney, 2000-2005. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
*/



namespace hz {



namespace internal {


	/// stream_cast helper stream
	template<typename Target, typename Source>
	class lexical_stream {
		public:

			// Constructor
			lexical_stream()
			{
				stream.unsetf(std::ios::skipws);  // disable whitespace skipping

				if (std::numeric_limits<Target>::is_specialized) {
					stream.precision(std::numeric_limits<Target>::digits10 + 1);

				} else if (std::numeric_limits<Source>::is_specialized) {
					stream.precision(std::numeric_limits<Source>::digits10 + 1);
				}
			}

			/// Output operator
			bool operator<<(const Source &input)
			{
				return !(stream << input).fail();
			}

			/// Input operator
			template<typename InputStreamable>
			bool operator>>(InputStreamable &output)
			{
				return !std::is_pointer<InputStreamable>::value &&
					stream >> output &&
					stream.get() == std::char_traits<std::stringstream::char_type>::eof();
			}

			/// Input operator
			bool operator>>(std::string &output)
			{
				output = stream.str();
				return true;
			}

		private:
			std::stringstream stream;  ///< The actual string stream

	};


}  // ns internal




/// This is thrown in case of cast failure
class bad_stream_cast : public hz::bad_cast_except {
	public:
		bad_stream_cast(const std::type_info& src, const std::type_info& dest)
				: hz::bad_cast_except(src, dest, "bad_stream_cast",
				"Failed stream_cast from \"%s\" to \"%s\".")
		{ }
};



/// Stream cast - cast Source to Target using stream operators.
template<typename Target, typename Source>
Target stream_cast(const Source& arg)
{
	using NewSource = std::decay_t<Source>;

	internal::lexical_stream<Target, NewSource> interpreter;
	Target result;

	if (!(interpreter << arg && interpreter >> result)) {
		throw bad_stream_cast(typeid(NewSource), typeid(Target));
	}

	return result;
}




}  // ns


#endif

/// @}
