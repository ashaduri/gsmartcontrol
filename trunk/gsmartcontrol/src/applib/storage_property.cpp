/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include <map>
#include <ostream>  // not iosfwd - it doesn't work
#include <sstream>
#include <iomanip>
#include <locale>

#include "hz/string_num.h"  // number_to_string
#include "hz/stream_cast.h"  // stream_cast<>
#include "hz/format_unit.h"  // format_time_length
#include "hz/string_algo.h"  // string_join
#include "hz/string_num.h"  // number_to_string

#include "storage_property.h"



std::ostream& operator<< (std::ostream& os, const StorageCapability& p)
{
	os
			// << p.name << ": "
			<< p.flag_value;
	for (auto&& v : p.strvalues) {
		os << "\n\t" << v;
	}
	return os;
}



std::string StorageAttribute::format_raw_value() const
{
	// If it's fully a number, format it with commas
	if (hz::number_to_string_nolocale(raw_value_int) == raw_value) {
		std::stringstream ss;
		ss.imbue(std::locale(""));
		ss << std::fixed << raw_value_int;
		return ss.str();
	}
	return raw_value;
}



std::ostream& operator<< (std::ostream& os, const StorageAttribute& p)
{
//	os << p.name << ": "
	if (p.value.has_value()) {
		os << static_cast<int>(p.value.value());
	} else {
		os << "-";
	}
	os << " (" << p.format_raw_value() << ")";
	return os;
}



std::string StorageStatistic::format_value() const
{
	// If it's fully a number, format it with commas
	if (hz::number_to_string_nolocale(value_int) == value) {
		std::stringstream ss;
		ss.imbue(std::locale(""));
		ss << std::fixed << value_int;
		return ss.str();
	}
	return value;
}



std::ostream& operator<<(std::ostream& os, const StorageStatistic& p)
{
	os << p.value;
	return os;
}



std::string StorageErrorBlock::get_readable_error_types(const std::vector<std::string>& types)
{
	static const std::map<std::string, std::string> m = {
		{"ABRT", "Command aborted"},
		{"AMNF", "Address mark not found"},
		{"CCTO", "Command completion timed out"},
		{"EOM", "End of media"},
		{"ICRC", "Interface CRC error"},
		{"IDNF", "Identity not found"},
		{"ILI", "(Packet command-set specific)"},
		{"MC", "Media changed"},
		{"MCR", "Media change request"},
		{"NM", "No media"},
		{"obs", "Obsolete"},
		{"TK0NF", "Track 0 not found"},
		{"UNC", "Uncorrectable error in data"},
		{"WP", "Media is write protected"},
	};

	std::vector<std::string> sv;
	for (const auto& type : types) {
		if (m.find(type) != m.end()) {
			sv.push_back(m.at(type));
		} else {
			sv.push_back("[unknown type" + (type.empty() ? "" : (": " + type)) + "]");
		}
	}

	return hz::string_join(sv, ", ");
}



int StorageErrorBlock::get_warning_level_for_error_type(const std::string& type)
{
	static const std::map<std::string, StorageProperty::warning_t> m = {
		{"ABRT", StorageProperty::warning_none},
		{"AMNF", StorageProperty::warning_alert},
		{"CCTO", StorageProperty::warning_warn},
		{"EOM", StorageProperty::warning_warn},
		{"ICRC", StorageProperty::warning_warn},
		{"IDNF", StorageProperty::warning_alert},
		{"ILI", StorageProperty::warning_notice},
		{"MC", StorageProperty::warning_none},
		{"MCR", StorageProperty::warning_none},
		{"NM", StorageProperty::warning_none},
		{"obs", StorageProperty::warning_none},
		{"TK0NF", StorageProperty::warning_alert},
		{"UNC", StorageProperty::warning_alert},
		{"WP", StorageProperty::warning_none},
	};

	if (m.find(type) != m.end()) {
		return int(m.at(type));
	}
	return StorageProperty::warning_none;  // unknown error
}



std::string StorageErrorBlock::format_lifetime_hours() const
{
	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << std::fixed << lifetime_hours;
	return ss.str();
}



std::ostream& operator<< (std::ostream& os, const StorageErrorBlock& b)
{
	os << "Error number " << b.error_num << ": "
		<< hz::string_join(b.reported_types, ", ")
		<< " [" << StorageErrorBlock::get_readable_error_types(b.reported_types) << "]";
	return os;
}



std::string StorageSelftestEntry::format_lifetime_hours() const
{
	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << std::fixed << lifetime_hours;
	return ss.str();
}



std::ostream& operator<< (std::ostream& os, const StorageSelftestEntry& b)
{
	os << "Test entry " << b.test_num << ": "
		<< b.type << ", status: " << b.get_status_str() << ", remaining: " << int(b.remaining_percent);
	return os;
}




void StorageProperty::dump(std::ostream& os, std::size_t internal_offset) const
{
	std::string offset(internal_offset, ' ');

	os << offset << "[" << get_section_name(section)
			<< (section == section_data ? (", " + get_subsection_name(subsection)) : "") << "]"
			<< " " << generic_name
			// << (generic_name == reported_name ? "" : (" (" + reported_name + ")"))
			<< ": [" << get_value_type_name() << "] ";

	// if (!readable_value.empty())
	// 	os << readable_value;

	if (std::holds_alternative<std::monostate>(value)) {
		os << "[empty]";
	} else if (std::holds_alternative<std::string>(value)) {
		os << std::get<std::string>(value);
	} else if (std::holds_alternative<int64_t>(value)) {
		os << std::get<int64_t>(value) << " [" << reported_value << "]";
	} else if (std::holds_alternative<bool>(value)) {
		os << std::string(std::get<bool>(value) ? "Yes" : "No") << " [" << reported_value << "]";
	} else if (std::holds_alternative<std::chrono::seconds>(value)) {
		os << std::get<std::chrono::seconds>(value).count() << " sec [" << reported_value << "]";
	} else if (std::holds_alternative<StorageCapability>(value)) {
		os << std::get<StorageCapability>(value);
	} else if (std::holds_alternative<StorageAttribute>(value)) {
		os << std::get<StorageAttribute>(value);
	} else if (std::holds_alternative<StorageStatistic>(value)) {
		os << std::get<StorageStatistic>(value);
	} else if (std::holds_alternative<StorageErrorBlock>(value)) {
		os << std::get<StorageErrorBlock>(value);
	} else if (std::holds_alternative<StorageSelftestEntry>(value)) {
		os << std::get<StorageSelftestEntry>(value);
	}
}



std::string StorageProperty::format_value(bool add_reported_too) const
{
	if (!readable_value.empty())
		return readable_value;

	if (std::holds_alternative<std::monostate>(value))
		return "[unknown]";
	if (std::holds_alternative<std::string>(value))
		return std::get<std::string>(value);
	if (std::holds_alternative<int64_t>(value))
		return hz::number_to_string_locale(std::get<int64_t>(value)) + (add_reported_too ? (" [" + reported_value + "]") : "");
	if (std::holds_alternative<bool>(value))
		return std::string(std::get<bool>(value) ? "Yes" : "No") + (add_reported_too ? (" [" + reported_value + "]") : "");
	if (std::holds_alternative<std::chrono::seconds>(value))
		return hz::format_time_length(std::get<std::chrono::seconds>(value)) + (add_reported_too ? (" [" + reported_value + "]") : "");
	if (std::holds_alternative<StorageCapability>(value))
		return hz::stream_cast<std::string>(std::get<StorageCapability>(value));
	if (std::holds_alternative<StorageAttribute>(value))
		return hz::stream_cast<std::string>(std::get<StorageAttribute>(value));
	if (std::holds_alternative<StorageStatistic>(value))
		return hz::stream_cast<std::string>(std::get<StorageStatistic>(value));
	if (std::holds_alternative<StorageErrorBlock>(value))
		return hz::stream_cast<std::string>(std::get<StorageErrorBlock>(value));
	if (std::holds_alternative<StorageSelftestEntry>(value))
		return hz::stream_cast<std::string>(std::get<StorageSelftestEntry>(value));

	return "[error]";
}







/// @}
