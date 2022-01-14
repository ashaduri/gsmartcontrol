/******************************************************************************
License: Zlib
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_STRING_ALGO_H
#define HZ_STRING_ALGO_H

#include <string>
#include <cctype>  // std::tolower, std::toupper
#include <string_view>



namespace hz {



// --------------------------------------------- Split


/// Split a string into components by character (delimiter), appending the
/// components (without delimiters) to container "append_here".
/// If skip_empty is true, then empty components will be omitted.
/// If "limit" is more than 0, only put a maximum of "limit" number of
/// elements into vector, with the last one containing the rest of the string.
template<class Container>
void string_split(const std::string& str, char delimiter,
		Container& append_here, bool skip_empty = false, typename Container::difference_type limit = 0)
{
	std::string::size_type curr = 0, last = 0;
	std::string::size_type end = str.size();
	typename Container::difference_type num = 0;  // number inserted

	while (true) {
		if (last >= end) {  // last is past the end
			if (!skip_empty)  // no need to check num here
				append_here.push_back(std::string());
			break;
		}

		curr = str.find(delimiter, last);

		if (!skip_empty || (curr != last)) {
			if (++num == limit) {
				append_here.push_back(str.substr(last, std::string::npos));
				break;
			}
			append_here.push_back(str.substr(last, (curr == std::string::npos ? curr : (curr - last))));
		}

		if (curr == std::string::npos)
			break;

		last = curr + 1;
	}
}



/// Split a string into components by another string (delimiter), appending the
/// components (without delimiters) to container "append_here".
/// If skip_empty is true, then empty components will be omitted.
/// If "limit" is more than 0, only put a maximum of "limit" number of
/// elements into vector, with the last one containing the rest of the string.
template<class Container>
void string_split(const std::string& str, const std::string& delimiter,
		Container& append_here, bool skip_empty = false, typename Container::difference_type limit = 0)
{
	std::string::size_type skip_size = delimiter.size();
	if (str.size() < skip_size) {
		append_here.push_back(str);
		return;
	}

	std::string::size_type curr = 0, last = 0;
	std::string::size_type end = str.size();
	typename Container::difference_type num = 0;  // number inserted

	while (true) {
		if (last >= end) {  // last is past the end
			if (!skip_empty)  // no need to check num here
				append_here.push_back(std::string());
			break;
		}

		curr = str.find(delimiter, last);
		std::string component(str, last, (curr == std::string::npos ? curr : (curr - last)));

		if (!skip_empty || !component.empty()) {
			if (++num == limit) {
				append_here.push_back(str.substr(last, std::string::npos));
				break;
			}
			append_here.push_back(component);
		}

		if (curr == std::string::npos)
			break;

		last = curr + skip_size;
	}
}


/// Split a string into components by any of the characters (delimiters), appending the
/// components (without delimiters) to container "append_here".
/// If skip_empty is true, then empty components will be omitted.
/// If "limit" is more than 0, only put a maximum of "limit" number of
/// elements into vector, with the last one containing the rest of the string.
template<class Container>
void string_split_by_chars(const std::string& str, const std::string& delimiter_chars,
		Container& append_here, bool skip_empty = false, typename Container::difference_type limit = 0)
{
	std::string::size_type curr = 0, last = 0;
	std::string::size_type end = str.size();
	typename Container::difference_type num = 0;  // number inserted

	while (true) {
		if (last >= end) {  // last is past the end
			if (!skip_empty)  // no need to check num here
				append_here.push_back(std::string());
			break;
		}

		curr = str.find_first_of(delimiter_chars, last);

		if (!skip_empty || (curr != last)) {
			if (++num == limit) {
				append_here.push_back(str.substr(last, std::string::npos));
				break;
			}
			append_here.push_back(str.substr(last, (curr == std::string::npos ? curr : (curr - last))));
		}

		if (curr == std::string::npos)
			break;

		last = curr + 1;
	}
}




// --------------------------------------------- Join


/// Join elements of container v into a string, using glue between them.
template<class Container>
std::string string_join(const Container& v, char glue)
{
	std::string ret;
	for (auto begin = v.cbegin(), iter = begin; iter != v.cend(); ++iter) {
		if (iter != begin)
			ret += glue;
		ret += (*iter);
	}
	return ret;
}



/// Join elements of container v into a string, using glue between them.
template<class Container>
std::string string_join(const Container& v, const std::string_view& glue)
{
	std::string ret;
	for (auto begin = v.cbegin(), iter = begin; iter != v.cend(); ++iter) {
		if (iter != begin)
			ret += glue;
		ret += (*iter);
	}
	return ret;
}




// --------------------------------------------- Trim


/// Trim a string s from both sides (modifying s). Return true if s was modified.
/// Trimming removes all trim_chars that occur on either side of the string s.
inline bool string_trim(std::string& s, const std::string& trim_chars = " \t\r\n")
{
	if (trim_chars.empty())
		return false;

	const auto s_size = s.size();

	std::string::size_type index = s.find_last_not_of(trim_chars);
	if (index != std::string::npos)
		s.erase(index + 1);  // from index+1 to the end

	index = s.find_first_not_of(trim_chars);
	if (index != std::string::npos) {
		s.erase(0, index);
	} else {
		s.clear();
	}

	return s_size != s.size();  // true if s was modified
}


/// Trim a string s from both sides (not modifying s), returning the changed string.
/// Trimming removes all trim_chars that occur on either side of the string s.
inline std::string string_trim_copy(const std::string& s, const std::string& trim_chars = " \t\r\n")
{
	std::string ret(s);
	string_trim(ret, trim_chars);
	return ret;
}




/// Trim a string s from the left (modifying s). Return true if s was modified.
/// Trimming removes all trim_chars that occur on the left side of the string s.
inline bool string_trim_left(std::string& s, const std::string& trim_chars = " \t\r\n")
{
	if (trim_chars.empty())
		return false;

	const auto s_size = s.size();

	std::string::size_type index = s.find_first_not_of(trim_chars);
	if (index != std::string::npos) {
		s.erase(0, index);
	} else {
		s.clear();
	}

	return s_size != s.size();  // true if s was modified
}


/// Trim a string s from the left (not modifying s), returning the changed string.
/// Trimming removes all trim_chars that occur on the left side of the string s.
inline std::string string_trim_left_copy(const std::string& s, const std::string& trim_chars = " \t\r\n")
{
	std::string ret(s);
	string_trim_left(ret, trim_chars);
	return ret;
}




/// Trim a string s from the right (modifying s). Return true if s was modified.
/// Trimming removes all trim_chars that occur on the right side of the string s.
inline bool string_trim_right(std::string& s, const std::string& trim_chars = " \t\r\n")
{
	if (trim_chars.empty())
		return false;

	const std::string::size_type s_size = s.size();

	std::string::size_type index = s.find_last_not_of(trim_chars);
	if (index != std::string::npos)
		s.erase(index + 1);  // from index+1 to the end

	return s_size != s.size();  // true if s was modified
}


/// Trim a string s from the right (not modifying s), returning the changed string.
/// Trimming removes all trim_chars that occur on the right side of the string s.
inline std::string string_trim_right_copy(const std::string& s, const std::string& trim_chars = " \t\r\n")
{
	std::string ret(s);
	string_trim_right(ret, trim_chars);
	return ret;
}



// --------------------------------------------- Erase


/// Erase the left side of string s if it contains substring_to_erase,
/// modifying s. Returns true if s was modified.
inline bool string_erase_left(std::string& s, const std::string& substring_to_erase)
{
	if (substring_to_erase.empty())
		return false;

	std::string::size_type sub_size = substring_to_erase.size();
	if (s.compare(0, sub_size, substring_to_erase) == 0) {
		s.erase(0, sub_size);
		return true;
	}

	return false;
}


/// Erase the left side of string s if it contains substring_to_erase,
/// not modifying s, returning the changed string.
inline std::string string_erase_left_copy(const std::string& s, const std::string& substring_to_erase)
{
	std::string ret(s);
	string_erase_left(ret, substring_to_erase);
	return ret;
}




/// Erase the right side of string s if it contains substring_to_erase,
/// modifying s. Returns true if s was modified.
inline bool string_erase_right(std::string& s, const std::string& substring_to_erase)
{
	std::string::size_type sub_size = substring_to_erase.size();
	if (sub_size == 0)
		return false;

	std::string::size_type s_size = s.size();
	if (sub_size > s_size)
		return false;

	if (s.compare(s_size - sub_size, sub_size, substring_to_erase) == 0) {
		s.erase(s_size - sub_size, sub_size);
		return true;
	}

	return false;
}


/// Erase the right side of string s if it contains substring_to_erase,
/// not modifying s, returning the changed string.
inline std::string string_erase_right_copy(const std::string& s, const std::string& substring_to_erase)
{
	std::string ret(s);
	string_erase_right(ret, substring_to_erase);
	return ret;
}



// --------------------------------------------- Misc. Transformations


/// remove adjacent duplicate chars inside s (modifying s).
/// returns true if s was modified.
/// useful for e.g. removing extra spaces inside the string.
inline bool string_remove_adjacent_duplicates(std::string& s, char c, std::size_t max_out_adjacent = 1)
{
	if (s.size() <= max_out_adjacent)
		return false;

	bool changed = false;
	std::string::size_type pos1 = 0, pos2 = 0;

	while ((pos1 = s.find(c, pos1)) != std::string::npos) {
		pos2 = s.find_first_not_of(c, pos1);
		if (pos2 == std::string::npos)
			pos2 = s.size();  // just past the last char
		if (pos2 - pos1 > max_out_adjacent) {
			s.erase(pos1 + max_out_adjacent, pos2 - pos1 - max_out_adjacent);
			changed = true;
		}
		pos1 += max_out_adjacent;
	}

	return changed;
}


/// remove adjacent duplicate chars inside s, not modifying s, returning the changed string.
inline std::string string_remove_adjacent_duplicates_copy(const std::string& s, char c, std::size_t max_out_adjacent = 1)
{
	std::string ret(s);
	string_remove_adjacent_duplicates(ret, c, max_out_adjacent);
	return ret;
}




// --------------------------------------------- Replace


// TODO: Add string_replace_linear(), where multiple strings are replaced into the
// original string (as opposed to previous result).


/// Replace from with to inside s (modifying s). Return number of replacements made.
inline std::string::size_type string_replace(std::string& s,
		const std::string_view& from, const std::string_view& to, int max_replacements = -1)
{
	if (from.empty())
		return std::string::npos;
	if (max_replacements == 0 || from == to)
		return 0;

	const std::string::size_type from_len(from.size());
	const std::string::size_type to_len(to.size());

	std::string::size_type cnt = 0;
	std::string::size_type pos = 0;

	while ((pos = s.find(from, pos)) != std::string::npos) {
		s.replace(pos, from_len, to);
		pos += to_len;
		if (static_cast<int>(++cnt) >= max_replacements && max_replacements != -1)
			break;
	}

	return cnt;
}


/// Replace from with to inside s, not modifying s, returning the changed string.
inline std::string string_replace_copy(const std::string& s,
		const std::string_view& from, const std::string_view& to, int max_replacements = -1)
{
	std::string ret(s);
	string_replace(ret, from, to, max_replacements);
	return ret;
}




/// Replace from with to inside s (modifying s). char version.
inline std::string::size_type string_replace(std::string& s,
		char from, char to, int max_replacements = -1)
{
	if (max_replacements == 0 || from == to)
		return 0;

	std::string::size_type cnt = 0, pos = 0;

	while ((pos = s.find(from, pos)) != std::string::npos) {
		s[pos] = to;
		++pos;
		if (static_cast<int>(++cnt) >= max_replacements && max_replacements != -1)
			break;
	}

	return cnt;
}


/// Replace from with to inside s, not modifying s, returning the changed string. char version.
inline std::string string_replace_copy(const std::string& s,
		char from, char to, int max_replacements = -1)
{
	std::string ret(s);
	string_replace(ret, from, to, max_replacements);
	return ret;
}





/// Replace from_chars[0] with to_chars[0], from_chars[1] with to_chars[1], etc... in s (modifying s).
/// from_chars.size() must be equal to to_chars.size().
/// Note: This is a multi-pass algorithm (there are from_chars.size() iterations).
inline std::string::size_type string_replace_chars(std::string& s,
		const std::string_view& from_chars, const std::string_view& to_chars, int max_replacements = -1)
{
	const std::string::size_type from_size = from_chars.size();
	if (from_size != to_chars.size())
		return std::string::npos;
	if (max_replacements == 0 || from_chars == to_chars)
		return 0;

	std::string::size_type cnt = 0, pos = 0;

	for(std::string::size_type i = 0; i < from_size; ++i) {
		pos = 0;
		char from = from_chars[i], to = to_chars[i];
		while ((pos = s.find(from, pos)) != std::string::npos) {
			s[pos] = to;
			++pos;
			if (static_cast<int>(++cnt) >= max_replacements && max_replacements != -1)
				break;
		}
		if (max_replacements != -1 && static_cast<int>(cnt) >= max_replacements)
			break;
	}

	return cnt;
}


/// Replace from_chars[0] with to_chars[0], from_chars[1] with to_chars[1], etc... in s,
/// not modifying s, returning the changed string.
/// from_chars.size() must be equal to to_chars.size().
/// Note: This is a multi-pass algorithm (there are from_chars.size() iterations).
inline std::string string_replace_chars_copy(const std::string& s,
		const std::string_view& from_chars, const std::string_view& to_chars, int max_replacements = -1)
{
	std::string ret(s);
	string_replace_chars(ret, from_chars, to_chars, max_replacements);
	return ret;
}





/// Replace all chars from from_chars with to_char (modifying s).
inline std::string::size_type string_replace_chars(std::string& s,
		const std::string_view& from_chars, char to_char, int max_replacements = -1)
{
	if (from_chars.empty())
		return std::string::npos;
	if (max_replacements == 0)
		return 0;

	std::string::size_type cnt = 0, pos = 0;
	while ((pos = s.find_first_of(from_chars, pos)) != std::string::npos) {
		s[pos] = to_char;
		++pos;
		if (static_cast<int>(++cnt) >= max_replacements && max_replacements != -1)
			break;
	}

	return cnt;
}


/// Replace all chars from from_chars with to_char, not modifying s, returning the changed string.
inline std::string string_replace_chars_copy(const std::string& s,
		const std::string_view& from_chars, char to_char, int max_replacements = -1)
{
	std::string ret(s);
	string_replace_chars(ret, from_chars, to_char, max_replacements);
	return ret;
}





/// Replace from_strings[0] with to_strings[0], from_strings[1] with to_strings[1], etc...
/// in s (modifying s). Returns total number of replacements performed.
/// from_strings.size() must be equal to to_strings.size().
/// Note: This is a multi-pass algorithm (there are from_strings.size() iterations).

/// Implementation note: We cannot use "template<template<class> C1>", because
/// it appears that it was a gcc extension, removed in 4.1 (C1 cannot bind to std::vector,
/// which has 2 (or more, implementation dependent) template parameters. gcc 4.1
/// allowed this because vector's other parameters have defaults).
template<class Container1, class Container2>
std::string::size_type string_replace_array(std::string& s,
		const Container1& from_strings, const Container2& to_strings, int max_replacements = -1)
{
	const std::string::size_type from_array_size = from_strings.size();

	if (from_array_size != to_strings.size())
		return std::string::npos;
	if (max_replacements == 0)
		return 0;

	std::string::size_type cnt = 0;

	for(std::string::size_type i = 0; i < from_array_size; ++i) {
		std::string::size_type pos = 0;
		while ((pos = s.find(from_strings[i], pos)) != std::string::npos) {
			s.replace(pos, from_strings[i].size(), to_strings[i]);
			pos += to_strings[i].size();
			if (static_cast<int>(++cnt) >= max_replacements && max_replacements != -1)
				break;
		}
		if (max_replacements != -1 && static_cast<int>(cnt) > max_replacements)
			break;
	}

	return cnt;
}


/// Eeplace from_strings[0] with to_strings[0], from_strings[1] with to_strings[1], etc... in s,
/// not modifying s, returning the changed string.
/// from_strings.size() must be equal to to_strings.size().
/// Note: This is a multi-pass algorithm (there are from_strings.size() iterations).
template<class Container1, class Container2> inline
std::string string_replace_array_copy(const std::string& s,
		const Container1& from_strings, const Container2& to_strings, int max_replacements = -1)
{
	std::string ret(s);
	string_replace_array(ret, from_strings, to_strings, max_replacements);
	return ret;
}


/// A version with a hash
template<class AssociativeContainer>
std::string::size_type string_replace_array(std::string& s,
		const AssociativeContainer& replacement_map, int max_replacements = -1)
{
	if (max_replacements == 0)
		return 0;

	std::string::size_type cnt = 0;

	for(const auto& iter : replacement_map) {
		std::string::size_type pos = 0;
		while ((pos = s.find(iter.first, pos)) != std::string::npos) {
			s.replace(pos, iter.first.size(), iter.second);
			pos += iter.second.size();
			if (static_cast<int>(++cnt) >= max_replacements && max_replacements != -1)
				break;
		}
		if (max_replacements != -1 && static_cast<int>(cnt) > max_replacements)
			break;
	}

	return cnt;
}


/// A version with a hash
template<class AssociativeContainer>
std::string string_replace_array_copy(const std::string& s,
		const AssociativeContainer& replacement_map, int max_replacements = -1)
{
	std::string ret(s);
	string_replace_array(ret, replacement_map, max_replacements);
	return ret;
}







/// Replace all strings in from_strings with to_string in s (modifying s).
/// Returns total number of replacements performed.
/// Note: This is a one-pass algorithm.
template<class Container> inline
std::string::size_type string_replace_array(std::string& s,
		const Container& from_strings, const std::string_view& to_string, int max_replacements = -1)
{
	const std::string::size_type from_array_size = from_strings.size();
	const std::string::size_type to_str_size = to_string.size();

	if (from_array_size == 0)
		return std::string::npos;
	if (max_replacements == 0)
		return 0;

	std::string::size_type cnt = 0, pos = 0;
	for(std::string::size_type i = 0; i < from_array_size; ++i) {
		pos = 0;
		while ((pos = s.find(from_strings[i], pos)) != std::string::npos) {
			s.replace(pos, from_strings[i].size(), to_string);
			pos += to_str_size;
			if (static_cast<int>(++cnt) >= max_replacements && max_replacements != -1)
				break;
		}
		if (max_replacements != -1 && static_cast<int>(cnt) > max_replacements)
			break;
	}

	return cnt;
}


/// Replace all strings in from_strings with to_string in s, not modifying s, returning the changed string.
/// Note: This is a one-pass algorithm.
template<class Container> inline
std::string string_replace_array_copy(const std::string& s,
		const Container& from_strings, const std::string_view& to_string, int max_replacements = -1)
{
	std::string ret(s);
	string_replace_array(ret, from_strings, to_string, max_replacements);
	return ret;
}




/// Same as the other overloads, but needed to avoid conflict with all-template version
template<class Container> inline
std::string::size_type string_replace_array(std::string& s,
		const Container& from_strings, const std::string& to_string, int max_replacements = -1)
{
	return string_replace_array<Container>(s, from_strings, std::string_view(to_string), max_replacements);
}


// Same as the other overloads, but needed to avoid conflict with all-template version
template<class Container> inline
std::string string_replace_array_copy(const std::string& s,
		const Container& from_strings, const std::string& to_string, int max_replacements = -1)
{
	return string_replace_array_copy<Container>(s, from_strings, std::string_view(to_string), max_replacements);
}




/// Same as the other overloads, but needed to avoid conflict with all-template version
template<class Container> inline
std::string::size_type string_replace_array(std::string& s,
		const Container& from_strings, const char* to_string, int max_replacements = -1)
{
	return string_replace_array<Container>(s, from_strings, std::string_view(to_string), max_replacements);
}


// Same as the other overloads, but needed to avoid conflict with all-template version
template<class Container> inline
std::string string_replace_array_copy(const std::string& s,
		const Container& from_strings, const char* to_string, int max_replacements = -1)
{
	return string_replace_array_copy<Container>(s, from_strings, std::string_view(to_string), max_replacements);
}




// --------------------------------------------- Matching


// FIXME These should be in C++20.


/// Check whether a string begins with another string
inline bool string_begins_with(const std::string& str, const std::string& substr)
{
	if (str.length() >= substr.length()) {
		return (str.compare(0, substr.length(), substr) == 0);
	}
	return false;
}



/// Check whether a string begins with a character
inline bool string_begins_with(const std::string& str, char ch)
{
	return !str.empty() && str[0] == ch;
}



/// Check whether a string ends with another string
inline bool string_ends_with(const std::string& str, const std::string& substr)
{
	if (str.length() >= substr.length()) {
		return (str.compare(str.length() - substr.length(), substr.length(), substr) == 0);
	}
	return false;
}



/// Check whether a string ends with a character
inline bool string_ends_with(const std::string& str, char ch)
{
	return !str.empty() && str[str.size() - 1] == ch;
}




// --------------------------------------------- Utility


/// Auto-detect and convert mac/dos/unix newline formats in s (modifying s) to unix format.
/// Returns true if \c s was changed.
inline bool string_any_to_unix(std::string& s)
{
	std::string::size_type n = hz::string_replace(s, "\r\n", "\n");  // dos
	n += hz::string_replace(s, '\r', '\n');  // mac
	return bool(n);
}



/// Auto-detect and convert mac/dos/unix newline formats in s to unix format.
/// Returns the result string.
inline std::string string_any_to_unix_copy(const std::string& s)
{
	std::string ret(s);
	string_any_to_unix(ret);
	return ret;
}


/// Auto-detect and convert mac/dos/unix newline formats in s (modifying s) to dos format.
/// Returns true if \c s was changed.
inline bool string_any_to_dos(std::string& s)
{
	bool changed = string_any_to_unix(s);
	std::string::size_type n = hz::string_replace(s, "\n", "\r\n");  // dos
	return bool(n) || changed;  // may not really work
}



/// Auto-detect and convert mac/dos/unix newline formats in s to dos format.
/// Returns the result string.
inline std::string string_any_to_dos_copy(const std::string& s)
{
	std::string ret(s);
	string_any_to_dos(ret);
	return ret;
}



/// Convert s to lowercase (modifying s). Return size of the string.
inline std::string::size_type string_to_lower(std::string& s)
{
	const std::string::size_type len = s.size();
	for(std::string::size_type i = 0; i != len; ++i) {
		s[i] = static_cast<char>(std::tolower(s[i]));
	}
	return len;
}



/// Convert s to lowercase, not modifying s, returning the changed string.
inline std::string string_to_lower_copy(const std::string& s)
{
	std::string ret(s);
	string_to_lower(ret);
	return ret;
}



/// Convert s to uppercase (modifying s). Return size of the string.
inline std::string::size_type string_to_upper(std::string& s)
{
	const std::string::size_type len = s.size();
	for(std::string::size_type i = 0; i != len; ++i) {
		s[i] = static_cast<char>(std::toupper(s[i]));
	}
	return len;
}



/// Convert s to uppercase, not modifying s, returning the changed string.
inline std::string string_to_upper_copy(const std::string& s)
{
	std::string ret(s);
	string_to_upper(ret);
	return ret;
}





}  // ns




#endif

/// @}
