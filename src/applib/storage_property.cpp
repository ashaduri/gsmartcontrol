/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include "local_glibmm.h"
#include <cstddef>
#include <chrono>
#include <ios>
#include <map>
#include <ostream>  // not iosfwd - it doesn't work
#include <sstream>
#include <locale>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>


#include "hz/string_num.h"  // number_to_string
#include "hz/stream_cast.h"  // stream_cast<>
#include "hz/format_unit.h"  // format_time_length
#include "hz/string_algo.h"  // string_join

#include "storage_property.h"



std::ostream& operator<< (std::ostream& os, const AtaStorageTextCapability& p)
{
	os
			// << p.name << ": "
			<< p.flag_value;
	for (auto&& v : p.strvalues) {
		os << "\n\t" << v;
	}
	return os;
}



std::string AtaStorageAttribute::get_readable_attribute_type_name(AttributeType type)
{
	static const std::unordered_map<AttributeType, std::string> m {
			{AttributeType::Unknown, "[unknown]"},
			{AttributeType::Prefail, "pre-failure"},
			{AttributeType::OldAge,  "old age"},
	};
	if (auto iter = m.find(type); iter != m.end()) {
		return iter->second;
	}
	return "[internal_error]";
}



std::string AtaStorageAttribute::get_readable_update_type_name(UpdateType type)
{
	static const std::unordered_map<UpdateType, std::string> m {
			{UpdateType::Unknown, "[unknown]"},
			{UpdateType::Always,  "continuously"},
			{UpdateType::Offline, "on offline data collect."},
	};
	if (auto iter = m.find(type); iter != m.end()) {
		return iter->second;
	}
	return "[internal_error]";
}



std::string AtaStorageAttribute::get_readable_fail_time_name(FailTime type)
{
	static const std::unordered_map<FailTime, std::string> m {
			{FailTime::Unknown, "[unknown]"},
			{FailTime::None,    "never"},
			{FailTime::Past,    "in the past"},
			{FailTime::Now,     "now"},
	};
	if (auto iter = m.find(type); iter != m.end()) {
		return iter->second;
	}
	return "[internal_error]";
}



std::string AtaStorageAttribute::format_raw_value() const
{
	// If it's fully a number, format it with commas
	if (hz::number_to_string_nolocale(raw_value_int) == raw_value) {
		std::stringstream ss;
		try {
			ss.imbue(std::locale(""));
		}
		catch (const std::runtime_error& e) {
			// something is wrong with system locale, can't do anything here.
		}
		ss << std::fixed << raw_value_int;
		return ss.str();
	}
	return raw_value;
}



std::ostream& operator<< (std::ostream& os, const AtaStorageAttribute& p)
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



bool AtaStorageStatistic::is_normalized() const
{
	return flags.find('N') != std::string::npos;
}



std::string AtaStorageStatistic::format_value() const
{
	// If it's fully a number, format it with commas
	if (hz::number_to_string_nolocale(value_int) == value) {
		std::stringstream ss;
		try {
			ss.imbue(std::locale(""));
		}
		catch (const std::runtime_error& e) {
			// something is wrong with system locale, can't do anything here.
		}
		ss << std::fixed << value_int;
		return ss.str();
	}
	return value;
}



std::ostream& operator<<(std::ostream& os, const AtaStorageStatistic& p)
{
	os << p.value;
	return os;
}



std::string AtaStorageErrorBlock::format_readable_error_types(const std::vector<std::string>& types)
{
	static const std::map<std::string, std::string> m = {
		{"ABRT", _("Command aborted")},
		{"AMNF", _("Address mark not found")},
		{"CCTO", _("Command completion timed out")},
		{"EOM", _("End of media")},
		{"ICRC", _("Interface CRC error")},
		{"IDNF", _("Identity not found")},
		{"ILI", _("(Packet command-set specific)")},
		{"MC", _("Media changed")},
		{"MCR", _("Media change request")},
		{"NM", _("No media")},
		{"obs", _("Obsolete")},
		{"TK0NF", _("Track 0 not found")},
		{"UNC", _("Uncorrectable error in data")},
		{"WP", _("Media is write protected")},
	};

	std::vector<std::string> sv;
	for (const auto& type : types) {
		if (m.find(type) != m.end()) {
			sv.push_back(m.at(type));
		} else {
			std::string name = _("Unknown type");
			if (!type.empty()) {
				name = Glib::ustring::compose(_("Unknown type: %1"), type);
			}
			sv.push_back(name);
		}
	}

	return hz::string_join(sv, _(", "));
}



WarningLevel AtaStorageErrorBlock::get_warning_level_for_error_type(const std::string& type)
{
	static const std::map<std::string, WarningLevel> m = {
		{"ABRT", WarningLevel::None},
		{"AMNF", WarningLevel::Alert},
		{"CCTO", WarningLevel::Warning},
		{"EOM", WarningLevel::Warning},
		{"ICRC", WarningLevel::Warning},
		{"IDNF", WarningLevel::Alert},
		{"ILI", WarningLevel::Notice},
		{"MC", WarningLevel::None},
		{"MCR", WarningLevel::None},
		{"NM", WarningLevel::None},
		{"obs", WarningLevel::None},
		{"TK0NF", WarningLevel::Alert},
		{"UNC", WarningLevel::Alert},
		{"WP", WarningLevel::None},
	};

	if (m.find(type) != m.end()) {
		return m.at(type);
	}
	return WarningLevel::None;  // unknown error
}



std::ostream& operator<< (std::ostream& os, const AtaStorageErrorBlock& b)
{
	os << "Error number " << b.error_num << ": "
		<< hz::string_join(b.reported_types, ", ")
		<< " [" << AtaStorageErrorBlock::format_readable_error_types(b.reported_types) << "]";
	return os;
}



std::string AtaStorageSelftestEntry::get_readable_status_name(Status s)
{
	static const std::unordered_map<Status, std::string> m {
			{Status::Unknown,                "[unknown]"},
			{Status::CompletedNoError,       "Completed without error"},
			{Status::AbortedByHost,          "Manually aborted"},
			{Status::Interrupted,            "Interrupted (host reset)"},
			{Status::FatalOrUnknown,         "Fatal or unknown error"},
			{Status::ComplUnknownFailure,    "Completed with unknown failure"},
			{Status::ComplElectricalFailure, "Completed with electrical failure"},
			{Status::ComplServoFailure,      "Completed with servo/seek failure"},
			{Status::ComplReadFailure,       "Completed with read failure"},
			{Status::ComplHandlingDamage,    "Completed: handling damage"},
			{Status::InProgress,             "In progress"},
			{Status::Reserved,               "Unknown / reserved state"},
	};
	if (auto iter = m.find(s); iter != m.end()) {
		return iter->second;
	}
	return "[internal_error]";
}



std::string AtaStorageSelftestEntry::get_readable_status() const
{
	return (status == Status::Unknown ? status_str : get_readable_status_name(status));
}



std::ostream& operator<< (std::ostream& os, const AtaStorageSelftestEntry& b)
{
	os << "Test entry " << b.test_num << ": "
		<< b.type << ", status: " << b.get_readable_status() << ", remaining: " << int(b.remaining_percent);
	return os;
}



std::ostream& operator<<(std::ostream& os, const NvmeStorageSelftestEntry& b)
{
	return os << "Test entry " << b.test_num << ": "
		<< NvmeSelfTestTypeExt::get_storable_name(b.type)
		<< ", result: " << NvmeSelfTestResultTypeExt::get_storable_name(b.result)
		<< ", power on hours: " << int(b.power_on_hours)
		<< ", lba: " << int(b.lba.value_or(0));
}



std::string StorageProperty::get_storable_value_type_name() const
{
	if (std::holds_alternative<std::monostate>(value))
		return "empty";
	if (std::holds_alternative<std::string>(value))
		return "string";
	if (std::holds_alternative<std::int64_t>(value))
		return "integer";
	if (std::holds_alternative<bool>(value))
		return "bool";
	if (std::holds_alternative<std::chrono::seconds>(value))
		return "time_length";
	if (std::holds_alternative<AtaStorageTextCapability>(value))
		return "capability";
	if (std::holds_alternative<AtaStorageAttribute>(value))
		return "attribute";
	if (std::holds_alternative<AtaStorageStatistic>(value))
		return "statistic";
	if (std::holds_alternative<AtaStorageErrorBlock>(value))
		return "error_block";
	if (std::holds_alternative<AtaStorageSelftestEntry>(value))
		return "ata_selftest_entry";
	if (std::holds_alternative<NvmeStorageSelftestEntry>(value))
		return "nvme_selftest_entry";
	return "[internal_error]";
}



bool StorageProperty::empty() const
{
	return std::holds_alternative<std::monostate>(value);
}



void StorageProperty::dump(std::ostream& os, std::size_t internal_offset) const
{
	const std::string offset(internal_offset, ' ');

	os << offset << "[" << StoragePropertySectionExt::get_storable_name(section) << "]"
			<< " " << generic_name
			// << (generic_name == reported_name ? "" : (" (" + reported_name + ")"))
			<< ": [" << get_storable_value_type_name() << "] ";

	// if (!readable_value.empty())
	// 	os << readable_value;

	if (std::holds_alternative<std::monostate>(value)) {
		os << "[empty]";
	} else if (std::holds_alternative<std::string>(value)) {
		os << std::get<std::string>(value);
	} else if (std::holds_alternative<std::int64_t>(value)) {
		os << std::get<std::int64_t>(value) << " [" << reported_value << "]";
	} else if (std::holds_alternative<bool>(value)) {
		os << std::string(std::get<bool>(value) ? "Yes" : "No") << " [" << reported_value << "]";
	} else if (std::holds_alternative<std::chrono::seconds>(value)) {
		os << std::get<std::chrono::seconds>(value).count() << " sec [" << reported_value << "]";
	} else if (std::holds_alternative<AtaStorageTextCapability>(value)) {
		os << std::get<AtaStorageTextCapability>(value);
	} else if (std::holds_alternative<AtaStorageAttribute>(value)) {
		os << std::get<AtaStorageAttribute>(value);
	} else if (std::holds_alternative<AtaStorageStatistic>(value)) {
		os << std::get<AtaStorageStatistic>(value);
	} else if (std::holds_alternative<AtaStorageErrorBlock>(value)) {
		os << std::get<AtaStorageErrorBlock>(value);
	} else if (std::holds_alternative<AtaStorageSelftestEntry>(value)) {
		os << std::get<AtaStorageSelftestEntry>(value);
	} else if (std::holds_alternative<NvmeStorageSelftestEntry>(value)) {
		os << std::get<NvmeStorageSelftestEntry>(value);
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
	if (std::holds_alternative<AtaStorageTextCapability>(value))
		return hz::stream_cast<std::string>(std::get<AtaStorageTextCapability>(value));
	if (std::holds_alternative<AtaStorageAttribute>(value))
		return hz::stream_cast<std::string>(std::get<AtaStorageAttribute>(value));
	if (std::holds_alternative<AtaStorageStatistic>(value))
		return hz::stream_cast<std::string>(std::get<AtaStorageStatistic>(value));
	if (std::holds_alternative<AtaStorageErrorBlock>(value))
		return hz::stream_cast<std::string>(std::get<AtaStorageErrorBlock>(value));
	if (std::holds_alternative<AtaStorageSelftestEntry>(value))
		return hz::stream_cast<std::string>(std::get<AtaStorageSelftestEntry>(value));
	if (std::holds_alternative<NvmeStorageSelftestEntry>(value))
		return hz::stream_cast<std::string>(std::get<NvmeStorageSelftestEntry>(value));

	return "[internal_error]";
}



std::string StorageProperty::get_description(bool clean) const
{
	if (clean)
		return this->description;
	return (this->description.empty() ? "No description available" : this->description);
}



void StorageProperty::set_description(const std::string& descr)
{
	this->description = descr;
}



void StorageProperty::set_name(const std::string& rep_name, const std::string& gen_name, const std::string& read_name)
{
	this->reported_name = rep_name;
	this->generic_name = (gen_name.empty() ? this->reported_name : gen_name);
	this->displayable_name = (read_name.empty() ? this->reported_name : read_name);
}



std::ostream& operator<<(std::ostream& os, const StorageProperty& p)
{
	p.dump(os);
	return os;
}







/// @}
