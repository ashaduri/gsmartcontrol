/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_BIN2ASCII_ENCODER_H
#define HZ_BIN2ASCII_ENCODER_H

#include "hz_config.h"  // feature macros

#include <cstddef>  // std::size_t
#include <string>
#include <cstring>  // std::strchr



namespace hz {


/// A class to convert a binary string to ascii string, while still retaining
/// ascii character readability. The result can be put inside double quotes.
class Bin2AsciiEncoder {
	public:

		/// Constructor.
		/// \param url_mode This enables URL-style encoding (not sure if it's correct, though).
		Bin2AsciiEncoder(bool url_mode = false) : url_mode_(url_mode)
		{ }


		/// Encode the passed string (src.data()).
		inline std::string encode(const std::string& src) const;


		/// Decode the passed string (src.data()).
		/// \return empty string on failure (provided that src is not empty), decoded string on success.
		inline std::string decode(const std::string& src) const;


		/// Check whether the string can be an encoded string.
		bool string_is_encoded(const std::string& s) const
		{
			return (s.find_first_not_of(url_mode_ ?
					encoded_chars_url : encoded_chars) == std::string::npos);
		}


		/// Check whether a character can occur in an encoded string
		bool char_is_encoded(char c) const
		{
			return std::strchr((url_mode_ ?
					encoded_chars_url : encoded_chars), c);
		}


	private:

		/// Characters which may appear in encoded string (no-url mode).
		static constexpr auto encoded_chars =
			"!^&()_-+=|.<>%"
			"0123456789"
			"abcdefghijklmnopqrstuvwxyz"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

		/// Characters which may appear in encoded string (url mode). Note: '+' is a special case.
		static constexpr auto encoded_chars_url =
			"-_.!~*'()+%"
			"0123456789"
			"abcdefghijklmnopqrstuvwxyz"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ";


		bool url_mode_ = false;  ///< URL encoding mode (or not)


		/// Get a byte value of a hex digit
		char char_from_hex_digit(char c) const
		{
			if (c >= '0' && c <= '9') {
				return static_cast<char>(c - '0');
			}
			if (c >= 'A' && c <= 'F') {
				return static_cast<char>(c - 'A' + 10);
			}
			if (c >= 'a' && c <= 'f') {
				return static_cast<char>(c - 'a' + 10);
			}
			return 16;  // aka invalid
		}

};




inline std::string Bin2AsciiEncoder::encode(const std::string& src) const
{
	std::size_t src_size = src.size();
	std::string dest;
	dest.reserve(src_size * 3);  // maximum

	static const char* digits = "0123456789ABCDEF";

	char c = 0;
	for (std::size_t i = 0; i < src_size; ++i) {
		c = src[i];

		if (c == ' ') {
			dest += (url_mode_ ? "+" : "%20");

		// don't use isalnum & friends, they're locale-dependent.
		} else if (c != '%' && std::strchr((url_mode_ ?
				encoded_chars_url : encoded_chars), c)) {
			dest += c;

		} else {
			unsigned char uc = static_cast<unsigned char>(c);
			dest += '%';
			dest += digits[(uc >> 4) & 0x0F];
			dest += digits[uc & 0x0F];
		}
	}

	return dest;
}



inline std::string Bin2AsciiEncoder::decode(const std::string& src) const
{
	std::size_t src_size = src.size();
	std::string dest;
	dest.reserve(src_size);  // maximum

	char c = 0;
	for (std::size_t i = 0; i < src_size; ++i) {
		c = src[i];

		if (c == '+' && url_mode_) {  // if not url_mode, treat it as the others
			dest += ' ';
			continue;
		}

		if (c == '%') {
			if ((i + 2) < src_size) {
				const char cv1 = char_from_hex_digit(src[++i]);
				const char cv2 = char_from_hex_digit(src[++i]);
				if (cv1 == 16 || cv2 == 16)  // invalid, max hex char + 1.
					return std::string();

				dest += static_cast<char>(cv2 + (cv1 << 4));
				continue;
			}
			return std::string();
		}

		dest += c;
	}

	return dest;
}







}  // ns




#endif

/// @}
