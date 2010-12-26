/**************************************************************************
 Copyright:
      (C) 2003 - 2009  Irakli Elizbarashvili <ielizbar 'at' gmail.com>
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>

 License:

 This software is provided 'as-is', without any express or implied
 warranty.  In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.
***************************************************************************/

#ifndef RMN_RESOURCE_SERIALIZATION_H
#define RMN_RESOURCE_SERIALIZATION_H

#include <string>  // std::string, std::getline()
#include <sstream>
#include <vector>
#include <istream>  // std::istream definition (calling bad() member)
#include <ostream>  // std::ostream definition (calling bad() member)
#include <ios>  // std::ios::*
#include <locale>  // std::locale
#include <fstream>
#include <stdint.h>

#include "hz/string_algo.h"  // string_split(), string_trim(), string_trim_copy()
#include "hz/string_num.h"  // string_is_numeric(), number_to_string()
#include "hz/bin2ascii_encoder.h"
#include "hz/debug.h"
#include "hz/hz_config.h"  // DISABLE_RTTI, RMN_* (global_macros.h)

#include "resource_node.h"
#include "resource_data_types.h"



namespace rmn {


// Unfortunately, these functions cannot accept
// resource_node<Data>::node_const_ptr
// as parameters, because of something called non-deducible
// context (regarding nested type).



// --------------------------- Version Stuff


// current serializer version
const char* version_identifier = "!rmn version ";
const int version_major = 0;
const int version_minor = 2;
const int version_revision = 1;



// get serializer version from string like "1.2.1"
inline bool serializer_version_from_string(const std::string& str, int& major, int& minor, int& revision)
{
	std::vector<std::string> v;
	hz::string_split(str, '.', v);
	if (v.size() < 3)
		return false;

	if (hz::string_is_numeric(v[0], major) && hz::string_is_numeric(v[0], minor)
			&& hz::string_is_numeric(v[0], revision)) {
		return true;
	}
	return false;
}



// generate serializer version from ints
inline std::string serializer_version_to_string(int major, int minor, int revision)
{
	std::stringstream ss;
	ss.imbue(std::locale::classic());
	ss << major << "." << minor << "." << revision;
	return ss.str();
}



// check if the version in string is the current supported one
inline bool serializer_check_version(const std::string& str, int& major, int& minor, int& revision)
{
	std::string version_string = serializer_version_to_string(version_major, version_minor, version_revision);
	std::string version_header = std::string("#") + version_identifier + version_string;

	// trim is needed to remove \r (happens when unserializing dos texts)
	if (hz::string_trim_copy(str).compare(0, version_header.size(), version_header) == 0)
		return true;

	return false;
}




// --------------------------- Types, etc...



// set _one_ node data, parsing a serialized string value
template<class Data> inline
bool resource_node_set_data_from_string(intrusive_ptr<resource_node<Data> > node,
		node_data_type type, const std::string& value_str)
{
	using hz::string_is_numeric;

	if (type == T_BOOL) { bool val = 0; if (string_is_numeric(value_str, val, true, true)) return node->set_data(val); return false; }  // with boolalpha
	if (type == T_INT32) { int32_t val = 0; if (string_is_numeric(value_str, val)) return node->set_data(val); return false; }
	if (type == T_UINT32) { uint32_t val = 0; if (string_is_numeric(value_str, val)) return node->set_data(val); return false; }
	if (type == T_INT64) { int64_t val = 0; if (string_is_numeric(value_str, val)) return node->set_data(val); return false; }
	if (type == T_UINT64) { uint64_t val = 0; if (string_is_numeric(value_str, val)) return node->set_data(val); return false; }
	if (type == T_DOUBLE) { double val = 0; if (string_is_numeric(value_str, val)) return node->set_data(val); return false; }
	if (type == T_FLOAT) { float val = 0; if (string_is_numeric(value_str, val)) return node->set_data(val); return false; }
	if (type == T_LDOUBLE) { long double val = 0; if (string_is_numeric(value_str, val)) return node->set_data(val); return false; }

	if (type == T_STRING) {
		if (value_str.size() >= 2 && value_str[0] == '"' && value_str[value_str.size() - 1] == '"') {
			std::string val = value_str.substr(1, value_str.size() - 2);
			if (!val.empty()) {
				hz::Bin2AsciiEncoder enc;
				val = enc.decode(val);  // returns empty string on error
				if (val.empty()) {
					debug_out_warn("rmn", "resource_node_set_data_from_string(): Error while decoding the data string.\n");
					return false;
				}
			}
			return node->set_data(val);
		}
		return false;
	}

	debug_out_error("rmn", "resource_node_set_data_from_string(): Error while reading data from string: Invalid type given.\n");
	return false;
}



inline std::string node_data_type_to_string(node_data_type type)
{
	switch(type) {
		case T_EMPTY: return "empty";  // not intended for general usage
		case T_BOOL: return "bool";
		case T_INT32: return "int32";
		case T_UINT32: return "uint32";
		case T_INT64: return "int64";
		case T_UINT64: return "uint64";
		case T_DOUBLE: return "double";
		case T_FLOAT: return "float";
		case T_LDOUBLE: return "ldouble";
		case T_STRING: return "string";
		case T_VOIDPTR: return "voidptr";
		case T_UNKNOWN: return "unknown";
	}
	return std::string();  // maybe same as unknown?
}



inline node_data_type node_data_type_from_string(const std::string& str)
{
	if (str == "empty") return T_EMPTY;
	if (str == "bool") return T_BOOL;
	if (str == "int32") return T_INT32;
	if (str == "int") return T_INT32;  // alias on int32
	if (str == "uint32") return T_UINT32;
	if (str == "int64") return T_INT64;
	if (str == "uint64") return T_UINT64;
	if (str == "double") return T_DOUBLE;
	if (str == "float") return T_FLOAT;
	if (str == "ldouble") return T_LDOUBLE;
	if (str == "string") return T_STRING;
	if (str == "voidptr") return T_VOIDPTR;
	if (str == "unknown") return T_UNKNOWN;  // along with ""

	return T_UNKNOWN;  // all others are treated as unknown type
}




// --------------------------- Saving



// Serializer works only if type tracking or RTTI is enabled
#if defined RMN_TYPE_TRACKING || !defined DISABLE_RTTI

#define RMN_SERIALIZE_AVAILABLE


// serialize _one_ node data to string.
template<class Data> inline
std::string serialize_node_data(intrusive_ptr<const resource_node<Data> > node)
{
	using hz::number_to_string;

	node_data_type type = resource_node_get_type(node);

	switch(type) {
		case T_BOOL:
			{ bool val = 0; if (node->get_data(val)) { return number_to_string(val, true); } break; }  // use boolalpha

		case T_INT32:
			{ int32_t val = 0; if (node->get_data(val)) { return number_to_string(val); } break; }
		case T_UINT32:
			{ uint32_t val = 0; if (node->get_data(val)) { return number_to_string(val); } break; }
		case T_INT64:
			{ int64_t val = 0; if (node->get_data(val)) { return number_to_string(val); } break; }
		case T_UINT64:
			{ uint64_t val = 0; if (node->get_data(val)) { return number_to_string(val); } break; }

		// the (non-fixed) precision here is std::numeric_limits<T>::digits10+1
		case T_DOUBLE:
			{ double val = 0; if (node->get_data(val)) { return number_to_string(val); } break; }
		case T_FLOAT:
			{ float val = 0; if (node->get_data(val)) { return number_to_string(val); } break; }
		case T_LDOUBLE:
			{ long double val = 0; if (node->get_data(val)) { return number_to_string(val); } break; }

		case T_STRING:
		{
			hz::Bin2AsciiEncoder enc;
			std::string val;
			if (node->get_data(val))
				return std::string("\"") + enc.encode(val) + "\"";  // there are no quotes inside.
			break;
		}

		case T_EMPTY: case T_VOIDPTR: case T_UNKNOWN:
			break;  // nofin'
	}

	// This is not an error, just an unsupported node type.
	return std::string();
}


template<class Data> inline
std::string serialize_node_data(intrusive_ptr<resource_node<Data> > node)
{
	return serialize_node_data(intrusive_ptr<const resource_node<Data> >(node));
}




namespace internal {


	// non-recursive, one-line serialization. returns "" if not serializable
	template<class Data> inline
	std::string serialize_node_to_string_helper(intrusive_ptr<const resource_node<Data> > node,
			const char* from_path = 0)
	{
		std::string str;
		if (!node)
			return str;

		std::string data_str = serialize_node_data(node);  // returns empty string if non-serializable
		if (data_str.empty())
			return str;

		std::string path = node->get_path();

		// if current node path contains from_path, remove it
		if (from_path && !path.compare(0, strlen(from_path), from_path)) {
			if (path.size() > (strlen(from_path) + 1)) {  // leading path + separator
				path.erase(0, strlen(from_path) + 1);
			}
		}

		if (path.empty()) {
			debug_out_error("rmn", "serialize_node_to_string(): Error: Unable to parse path: " << node->get_path() << "\n");
			return std::string();
		}

		std::string rval = node_data_type_to_string(resource_node_get_type(node)) + " " + data_str;

		return path + " = " + rval;
	}



	// version information is NOT prepended!
	template<class Data> inline
	bool serialize_node_to_stream_recursive_helper(intrusive_ptr<const resource_node<Data> > node,
			std::ostream& os, const char* from_path = 0)
	{
		if (!node) {
			debug_out_warn("rmn", "serialize_node_to_stream_recursive(): Empty node given!\n");
			return false;
		}

		std::string node_line = serialize_node_to_string_helper(node, from_path);
		if (!node_line.empty()) {
			os << node_line << "\n";
			if (os.bad()) {
				debug_out_error("rmn", "serialize_node_to_stream_recursive(): Error while writing to stream.\n");
				return false;
			}
		}

		if (!from_path) {  // make children save with relative paths to the first called node's path.
			from_path = node->get_path().c_str();
		}

		typename resource_node<Data>::child_const_iterator iter = node->children_begin();
		for ( ; iter != node->children_end(); ++iter) {
			serialize_node_to_stream_recursive_helper(intrusive_ptr<const resource_node<Data> >(*iter), os, from_path);
		}

		return true;
	}


}  // ns internal



// non-recursive, one-line serialization. returns "" if not serializable
template<class Data> inline
std::string serialize_node_to_string(intrusive_ptr<const resource_node<Data> > node)
{
	return internal::serialize_node_to_string_helper(node);
}


// non-recursive, one-line serialization. returns "" if not serializable
template<class Data> inline
std::string serialize_node_to_string(intrusive_ptr<resource_node<Data> > node)
{
	return serialize_node_to_string(intrusive_ptr<const resource_node<Data> >(node));
}



// from_path is internal. version information is NOT prepended!
template<class Data> inline
bool serialize_node_to_stream_recursive(intrusive_ptr<const resource_node<Data> > node, std::ostream& os)
{
	return internal::serialize_node_to_stream_recursive_helper(node, os);
}


// from_path is internal. version information is NOT prepended!
template<class Data> inline
bool serialize_node_to_stream_recursive(intrusive_ptr<resource_node<Data> > node, std::ostream& os)
{
	return serialize_node_to_stream_recursive(intrusive_ptr<const resource_node<Data> >(node), os);
}



// write node data to file recursively
template<class Data> inline
bool serialize_node_to_file_recursive(intrusive_ptr<const resource_node<Data> > node,
		const std::string& file)
{
	std::ofstream ofs(file.c_str(), std::ios::out | std::ios::binary);  // we always save in unix mode
	if (ofs.bad()) {
		debug_print_error("rmn", "serialize_node_to_file_recursive(): Unable to open file \"%s\"!\n", file.c_str());
		return false;
	}

	std::string version_string = serializer_version_to_string(version_major, version_minor, version_revision);
	ofs << "#" << version_identifier << version_string << "\n";

	debug_out_info("rmn", "Saving: \"" <<  node->get_path() << "\""
			<< " version: " << version_string << "\n");

	return serialize_node_to_stream_recursive(node, ofs);
}


// write node data to file recursively
template<class Data> inline
bool serialize_node_to_file_recursive(intrusive_ptr<resource_node<Data> > node,
		const std::string& file)
{
	return serialize_node_to_file_recursive(intrusive_ptr<const resource_node<Data> >(node), file);
}




// write node data to string recursively
template<class Data> inline
bool serialize_node_to_string_recursive(intrusive_ptr<const resource_node<Data> > node,
		std::string& put_here)
{
	std::stringstream ss;

	std::string version_string = serializer_version_to_string(version_major, version_minor, version_revision);
	ss << "#" << version_identifier << version_string << "\n";

	debug_out_info("rmn", "Saving: \"" <<  node->get_path() << "\""
			<< " version: " << version_string << "\n");

	if (!serialize_node_to_stream_recursive(node, ss))
		return false;

	put_here = ss.str();
	return true;
}


// write node data to file recursively
template<class Data> inline
bool serialize_node_to_string_recursive(intrusive_ptr<resource_node<Data> > node,
		std::string& put_here)
{
	return serialize_node_to_string_recursive(intrusive_ptr<const resource_node<Data> >(node), put_here);
}



#endif  // defined RMN_TYPE_TRACKING || !defined DISABLE_RTTI




// --------------------------- Loading



// paths supports only alphanumeric characters, '.' and '_' for components.
inline bool string_is_path(const std::string& s)
{
	if (s.empty())
		return false;

	hz::Bin2AsciiEncoder enc;

	for (std::string::const_iterator iter = s.begin(); iter != s.end(); ++iter) {
		if (!enc.char_is_encoded(*iter) && (*iter != PATH_DELIMITER))  // a safe name-char or a separator
			return false;
	}
	return true;
}




// create (or replace) a node from a serialized node line.
template<class Data> inline
typename resource_node<Data>::node_ptr
create_node_from_serialized_line(intrusive_ptr<resource_node<Data> > under_this_node,
		std::string line, int line_no)
{
	typename resource_node<Data>::node_ptr empty_node;
	hz::string_trim(line);  // remove \r, etc...

	if (line.empty() || line[0] == '#')  // "#" starts comment lines
		return empty_node;

	std::vector<std::string> components;
	hz::string_split(line, "=", components);

	if (components.size() != 2) {
		debug_out_warn("rmn", "Error while unserializing node on line " << line_no
				<< ": Invalid component count.\n");
		return empty_node;
	}

	std::string path = hz::string_trim_copy(components[0]);
	std::string rval = hz::string_trim_copy(components[1]);

	if (!string_is_path(path)) {
		debug_out_warn("rmn", "Error while unserializing node on line " << line_no
				<< ": The first component is not a valid path.\n");
		return empty_node;
	}

	components.clear();
	hz::string_split(rval, " ", components);


	node_data_type type;
	std::string value_str;
	if (components.size() == 2) {
		type = node_data_type_from_string(hz::string_trim_copy(components[0]));
		value_str = hz::string_trim_copy(components[1]);

		if (type == T_UNKNOWN) {
			debug_out_warn("rmn", "Error while unserializing node on line " << line_no
					<< " with path \"" << path << "\": The specified type is invalid.\n");
			return empty_node;
		}

	// try to autodetect the type
	} else if (components.size() == 1) {
		value_str = hz::string_trim_copy(components[0]);

		bool b = 0; int32_t i = 0; double d = 0;
		if (hz::string_is_numeric(value_str, b, true, true)) {  // true / false. use boolalpha.
			type = T_BOOL;

		} else if (hz::string_is_numeric(value_str, i)) {
			type = T_INT32;

		} else if (hz::string_is_numeric(value_str, d)) {
			type = T_DOUBLE;

		} else if (value_str.size() >= 2 && value_str[0] == '"' && value_str[value_str.size() - 1] == '"') {
			type = T_STRING;

		} else {
			debug_out_warn("rmn", "Error while unserializing node on line " << line_no
					<< " with path \"" << path << "\": Cannot auto-detect value type.\n");
			return empty_node;
		}

	} else {
		debug_out_warn("rmn", "Error while unserializing node on line " << line_no
				<< " with path \"" << path << "\": No value given.\n");
		return empty_node;
	}


	typename resource_node<Data>::node_ptr node_with_data(new resource_node<Data>);

	if (!resource_node_set_data_from_string(node_with_data, type, value_str)) {  // this won't touch the node if error
		debug_out_warn("rmn", "Error while unserializing node on line " << line_no
				<< " with path \"" << path << "\": Cannot convert the specified value to requested type.\n");
		return empty_node;
	}

	if (!under_this_node->build_nodes(path)) {
		debug_out_warn("rmn", "Error while unserializing node on line " << line_no
				<< " with path \"" << path << "\": Cannot build node.\n");  // may happen if the node is outside /config.
		return empty_node;
	}

	// Note: from this point, any error will leave the node path constructed!

	typename resource_node<Data>::node_ptr node = under_this_node->find_node(path);
	if (!node) {
		debug_out_error("rmn", "Error while unserializing node on line " << line_no
				<< " with path \"" << path << "\": Cannot read the just-built node!\n");  // internal error?
		return empty_node;
	}

	if (! node->copy_data_from(node_with_data)) {
		debug_out_error("rmn", "Error while unserializing node on line " << line_no
				<< " with path \"" << path << "\": Cannot copy data from temporary node!\n");  // internal error?
		return empty_node;
	}

	return node;
}






template<class Data> inline
bool unserialize_nodes_from_stream(intrusive_ptr<resource_node<Data> > under_this_node,
		std::istream& is)
{
	std::string line;
	// we use \n here, because we save in unix mode. however, if \r is encountered,
	// it will be trimmed afterwards (so dos texts can be loaded too).
	std::getline(is, line, '\n');  // first line, version string

	int major = 0; int minor = 0; int revision = 0;
	if (!serializer_check_version(line, major, minor, revision)) {  // invalid version
		debug_out_warn("rmn", "Error while loading nodes from data stream: "
				<< "The stream has invalid or unsupported version information.\n");
		return false;
	}

	int line_no = 1;  // line counter for diagnostics
	while (std::getline(is, line, '\n')) {
		++line_no;
		typename resource_node<Data>::node_ptr node =
			create_node_from_serialized_line(under_this_node, line, line_no);
	}

	return !is.bad();
}




template<class Data> inline
bool unserialize_nodes_from_string(intrusive_ptr<resource_node<Data> > under_this_node,
		const std::string& str)
{
	std::stringstream ss;
	ss << str;
	return unserialize_nodes_from_stream(under_this_node, ss);
}





template<class Data> inline
bool unserialize_nodes_from_file(intrusive_ptr<resource_node<Data> > under_this_node,
		const std::string& file)
{
	std::ifstream in(file.c_str(), std::ios::binary);
	if (in.bad())
		return false;

	return unserialize_nodes_from_stream(under_this_node, in);
}






} // namespace rmn



#endif
