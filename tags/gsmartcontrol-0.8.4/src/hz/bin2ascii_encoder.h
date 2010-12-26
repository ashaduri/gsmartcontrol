/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_BIN2ASCII_ENCODER_H
#define HZ_BIN2ASCII_ENCODER_H

#include "hz_config.h"  // feature macros

#include <string>
#include <cstring>  // strchr



/*
A class to convert a binary string to ascii string, while still retaining
ascii character readability. Safe to put inside double-quotes.

Note: The url_mode flag sets enables the URL-style encoding.
However, I'm not sure if it's correct.
*/


namespace hz {


class Bin2AsciiEncoder {

	public:

		Bin2AsciiEncoder(bool url_mode = false) : url_mode_(url_mode)
		{ }


		// encode the passed string (src.data()).
		inline std::string encode(const std::string& src) const;


		// decode the passed string (src.data()).
		// returns an empty string on failure (provided that src is not empty).
		inline std::string decode(const std::string& src) const;


		// check whether the string can be an encoded string
		bool string_is_encoded(const std::string& s) const
		{
			return (s.find_first_not_of(url_mode_ ?
					char_holder::encoded_chars_url : char_holder::encoded_chars) == std::string::npos);
		}


		// check whether the char can occur in an encoded string
		bool char_is_encoded(char c) const
		{
			return std::strchr((url_mode_ ?
					char_holder::encoded_chars_url : char_holder::encoded_chars), c);
		}



	private:


		template<typename Dummy>
		struct StaticHolder {
			// for description of these, see below.
			static const char* const encoded_chars;
			static const char* const encoded_chars_url;
		};

		typedef StaticHolder<void> char_holder;


		bool url_mode_;  // url encoding mode


		// helper function
		char char_from_hex_digit(char c) const
		{
			if (c >= '0' && c <= '9') {
				return (c - '0');
			}
			if (c >= 'A' && c <= 'F') {
				return (c - 'A' + 10);
			}
			if (c >= 'a' && c <= 'f') {
				return (c - 'a' + 10);
			}
			return 16;  // aka invalid
		}

};




// chars which may appear in encoded string.
template<typename Dummy>
const char* const Bin2AsciiEncoder::StaticHolder<Dummy>::encoded_chars =
	"!^&()_-+=|.<>%"
	"0123456789"
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ";


// same, but for url_mode. note: '+' is a special case.
template<typename Dummy>
const char* const Bin2AsciiEncoder::StaticHolder<Dummy>::encoded_chars_url =
	"-_.!~*'()+%"
	"0123456789"
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ";




// encode the passed string (src.data()).
inline std::string Bin2AsciiEncoder::encode(const std::string& src) const
{
	unsigned int src_size = src.size();
	std::string dest;
	dest.reserve(src_size * 3);  // maximum

	static const char* digits = "0123456789ABCDEF";

	char c = 0;
	for (unsigned int i = 0; i < src_size; ++i) {
		c = src[i];

		if (c == ' ') {
			dest += (url_mode_ ? "+" : "%20");

		// don't use isalnum & friends, they're locale-dependent.
		} else if (c != '%' && std::strchr((url_mode_ ?
				char_holder::encoded_chars_url : char_holder::encoded_chars), c)) {
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




// decode the passed string (src.data()).
// returns an empty string on failure (provided that src is not empty).
inline std::string Bin2AsciiEncoder::decode(const std::string& src) const
{
	unsigned int src_size = src.size();
	std::string dest;
	dest.reserve(src_size);  // maximum

	char c = 0;
	for (unsigned int i = 0; i < src_size; ++i) {
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

				dest += (cv2 + (cv1 << 4));
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
