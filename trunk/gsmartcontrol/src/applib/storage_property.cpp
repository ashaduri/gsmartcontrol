/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include <map>
#include <ostream>  // not iosfwd - it doesn't work

#include "hz/string_num.h"  // number_to_string
#include "hz/stream_cast.h"  // stream_cast<>
#include "hz/format_unit.h"  // format_time_length
#include "hz/string_algo.h"  // string_join

#include "storage_property.h"



std::ostream& operator<< (std::ostream& os, const StorageCapability& p)
{
	os
	// << p.name << ": "
	<< p.flag_value;
	for (StorageCapability::strvalue_list_t::const_iterator iter = p.strvalues.begin(); iter != p.strvalues.end(); ++iter) {
		os << "\n\t" << *iter;
	}
	return os;
}



std::ostream& operator<< (std::ostream& os, const StorageAttribute& p)
{
//	os << p.name << ": "
	if (p.value.defined()) {
		os << static_cast<int>(p.value.value());
	} else {
		os << "-";
	}
	os << " (" << p.raw_value_int << ")";
	return os;
}



std::string StorageErrorBlock::get_readable_error_types(const std::vector<std::string>& types)
{
	std::map<std::string, std::string> m;

	m["ABRT"] = "Command aborted";
	m["AMNF"] = "Address mark not found";
	m["CCTO"] = "Command completion timed out";
	m["EOM"] = "End of media";
	m["ICRC"] = "Interface CRC error";
	m["IDNF"] = "Identity not found";
	m["ILI"] = "(Packet command-set specific)";
	m["MC"] = "Media changed";
	m["MCR"] = "Media change request";
	m["NM"] = "No media";
	m["obs"] = "Obsolete";
	m["TK0NF"] = "Track 0 not found";
	m["UNC"] = "Uncorrectable error in data";
	m["WP"] = "Media is write protected";

	std::vector<std::string> sv;
	for (std::vector<std::string>::const_iterator iter = types.begin(); iter != types.end(); ++iter) {
		if (m.find(*iter) != m.end()) {
			sv.push_back(m[*iter]);
		} else {
			sv.push_back("[unknown type" + (iter->empty() ? "" : (": " + (*iter))) + "]");
		}
	}

	return hz::string_join(sv, ", ");
}



std::ostream& operator<< (std::ostream& os, const StorageErrorBlock& b)
{
	os << "Error number " << b.error_num << ": "
		<< hz::string_join(b.reported_types, ", ")
		<< " [" << StorageErrorBlock::get_readable_error_types(b.reported_types) << "]";
	return os;
}



std::ostream& operator<< (std::ostream& os, const StorageSelftestEntry& b)
{
	os << "Test entry " << b.test_num << ": "
		<< b.type << ", status: " << b.get_status_str() << ", remaining: " << int(b.remaining_percent);
	return os;
}




void StorageProperty::dump(std::ostream& os, int internal_offset) const
{
	std::string offset(internal_offset, ' ');

	os << offset << "[" << get_section_name(section)
		<< (section == section_data ? (", " + get_subsection_name(subsection)) : "") << "]"
		<< " " << generic_name
		// << (generic_name == reported_name ? "" : (" (" + reported_name + ")"))
		<< ": [" << get_value_type_name(value_type) << "] ";

	// if (!readable_value.empty())
	// 	os << readable_value;

	if (value_type == StorageProperty::value_type_unknown) {
		os << "[empty]";

	} else if (value_type == StorageProperty::value_type_string) {
		os << "\"" << value_string << "\"";

	} else if (value_type == StorageProperty::value_type_integer) {
		os << value_integer << " [" << reported_value << "]";

	} else if (value_type == StorageProperty::value_type_bool) {
		os << value_bool << " [" << reported_value << "]";

	} else if (value_type == StorageProperty::value_type_time_length) {
		os << value_time_length << " [" << reported_value << "]";

	} else if (value_type == StorageProperty::value_type_capability) {
		os << value_capability;

	} else if (value_type == StorageProperty::value_type_attribute) {
		os << value_attribute;

	} else if (value_type == StorageProperty::value_type_error_block) {
		os << value_error_block;

	} else if (value_type == StorageProperty::value_type_selftest_entry) {
		os << value_selftest_entry;
	}

}



std::string StorageProperty::format_value(bool add_reported_too) const
{
	if (!readable_value.empty())
		return readable_value;

	if (value_type == StorageProperty::value_type_unknown) {
		return "[unknown]";

	} else if (value_type == StorageProperty::value_type_string) {
		return value_string;

	} else if (value_type == StorageProperty::value_type_integer) {
		return hz::number_to_string(value_integer) + (add_reported_too ? (" [" + reported_value + "]") : "");

	} else if (value_type == StorageProperty::value_type_bool) {
		return std::string(value_bool ? "Yes" : "No") + (add_reported_too ? (" [" + reported_value + "]") : "");

	} else if (value_type == StorageProperty::value_type_time_length) {
		return hz::format_time_length(value_time_length) + (add_reported_too ? (" [" + reported_value + "]") : "");

	} else if (value_type == StorageProperty::value_type_capability) {
		return hz::stream_cast<std::string>(value_capability);

	} else if (value_type == StorageProperty::value_type_attribute) {
		return hz::stream_cast<std::string>(value_attribute);

	} else if (value_type == StorageProperty::value_type_error_block) {
		return hz::stream_cast<std::string>(value_error_block);

	} else if (value_type == StorageProperty::value_type_error_block) {
		return hz::stream_cast<std::string>(value_selftest_entry);
	}

	return "[error]";
}







/// @}
