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

#ifndef STORAGE_PROPERTY_H
#define STORAGE_PROPERTY_H

#include <string>
#include <vector>
#include <iosfwd>
#include <cstdint>
#include <optional>
#include <chrono>
#include <variant>
#include <unordered_map>

#include "warning_level.h"



/// Holds one block of "capabilities" subsection
/// (only for non-time-interval blocks).
class StorageCapability {
	public:
		std::string reported_flag_value;  ///< original flag value as a string
		uint16_t flag_value = 0x0;  ///< Flag value. This is one or sometimes two bytes (maybe more?)
		std::string reported_strvalue;  ///< Original flag descriptions
		std::vector<std::string> strvalues;  ///< A list of capabilities in the block.
};


/// Output operator for debug purposes
std::ostream& operator<< (std::ostream& os, const StorageCapability& p);





/// Holds one line of "attributes" subsection
class StorageAttribute {
	public:

		/// Disk type the attribute may match
		enum class DiskType {
			Any,  ///< Any disk type
			Hdd,  ///< HDD (rotational) only
			Ssd  ///< SSD only
		};

		/// Attribute pre-failure / old-age type
		enum class AttributeType {
			unknown,  ///< Unknown
			prefail,  ///< Pre-failure (reported: Pre-fail)
			old_age  ///< Old age (reported: Old_age)
		};

		/// Get readable attribute type name
		static std::string get_attr_type_name(AttributeType type)
		{
			static const std::unordered_map<AttributeType, std::string> m {
					{AttributeType::unknown, "[unknown]"},
					{AttributeType::prefail, "pre-failure"},
					{AttributeType::old_age, "old age"},
			};
			if (auto iter = m.find(type); iter != m.end()) {
				return iter->second;
			}
			return "[internal_error]";
		}


		/// Attribute when-updated type
		enum class UpdateType {
			unknown,  ///< Unknown
			always,  ///< Continuously (reported: Always)
			offline  ///< Only during offline data collection (reported: Offline)
		};

		/// Get readable when-updated type name
		static std::string get_update_type_name(UpdateType type)
		{
			static const std::unordered_map<UpdateType, std::string> m {
					{UpdateType::unknown, "[unknown]"},
					{UpdateType::always, "continuously"},
					{UpdateType::offline, "on offline data collect."},
			};
			if (auto iter = m.find(type); iter != m.end()) {
				return iter->second;
			}
			return "[internal_error]";
		}


		/// Attribute when-failed type
		enum class FailTime {
			unknown,  ///< Unknown
			none,  ///< Never (reported: -)
			past,  ///< In the past (reported: In_the_past)
			now  ///< Now (reported: FAILING_NOW)
		};

		/// Get a readable when-failed type name
		static std::string get_fail_time_name(FailTime type)
		{
			static const std::unordered_map<FailTime, std::string> m {
					{FailTime::unknown, "[unknown]"},
					{FailTime::none, "never"},
					{FailTime::past, "in the past"},
					{FailTime::now, "now"},
			};
			if (auto iter = m.find(type); iter != m.end()) {
				return iter->second;
			}
			return "[internal_error]";
		}


		/// Format raw value with commas (if it's a number)
		std::string format_raw_value() const;


		int32_t id = -1;  ///< Attribute ID (most vendors agree on this)
		std::string flag;  ///< "Old" format is "0xXXXX", "brief" format is "PO--C-".
		std::optional<uint8_t> value;  ///< Normalized value. May be unset ("---").
		std::optional<uint8_t> worst;  ///< Worst ever value. May be unset ("---").
		std::optional<uint8_t> threshold;  ///< Threshold for normalized value. May be unset ("---").
		AttributeType attr_type = AttributeType::unknown;  ///< Attribute pre-fail / old-age type
		UpdateType update_type = UpdateType::unknown;  ///< When-updated type
		FailTime when_failed = FailTime::unknown;  ///< When-failed type
		std::string raw_value;  ///< Raw value as a string, as presented by smartctl (formatted).
		int64_t raw_value_int = 0;  ///< Same as raw_value, but parsed as int64. original value is 6 bytes I think.

};


/// Output operator for debug purposes
std::ostream& operator<< (std::ostream& os, const StorageAttribute& p);




/// Holds one line of "devstat" subsection
class StorageStatistic {
	public:

		/// Whether the normalization flag is present
		bool is_normalized() const
		{
			return flags.find('N') != flags.npos;
		}

		/// Format value with commas (if it's a number)
		std::string format_value() const;

		bool is_header = false;  ///< If the line is a header
		std::string flags;  ///< Flags in "NDC" / "---" format
		std::string value;  ///< Value as a string, as presented by smartctl (formatted).
		int64_t value_int = 0;  ///< Same as value, but parsed as int64.
		int64_t page = 0;  ///< Page
		int64_t offset = 0;  ///< Offset in page
};


/// Output operator for debug purposes
std::ostream& operator<< (std::ostream& os, const StorageStatistic& p);



/// Holds one error block of "error log" subsection
class StorageErrorBlock {
	public:

		/// Get readable error types from reported types
		static std::string get_displayable_error_types(const std::vector<std::string>& types);

		/// Get warning level (Warning) for an error type
		static WarningLevel get_warning_level_for_error_type(const std::string& type);

		/// Format lifetime hours with comma
		std::string format_lifetime_hours() const;

		uint32_t error_num = 0;  ///< Error number
		uint32_t lifetime_hours = 0;  ///< When the error occurred (in lifetime hours)
		std::string device_state;  ///< Device state during the error - "active or idle", standby, etc...
		std::vector<std::string> reported_types;  ///< Array of reported types (strings), e.g. "UNC".
		std::string type_more_info;  ///< More info on error type (e.g. "at LBA = 0x0253eac0 = 39054016")
};


/// Output operator for debug purposes
std::ostream& operator<< (std::ostream& os, const StorageErrorBlock& b);




/// Holds one entry of selftest_log subsection.
/// Also, holds "Self-test execution status" capability's "internal" section version.
class StorageSelftestEntry {
	public:

		/// Self-test log entry status
		enum class Status {
			unknown,  ///< Initial state
			completed_no_error,  ///< Completed with no error, or no test was run
			aborted_by_host,  ///< Aborted by host
			interrupted,  ///< Interrupted by user
			fatal_or_unknown,  ///< Fatal or unknown error. Treated as test failure.
			compl_unknown_failure,  ///< Completed with unknown error. Treated as test failure.
			compl_electrical_failure,  ///< Completed with electrical error. Treated as test failure.
			compl_servo_failure,  ///< Completed with servo error. Treated as test failure.
			compl_read_failure,  ///< Completed with read error. Treated as test failure.
			compl_handling_damage,  ///< Completed with handling damage error. Treated as test failure.
			in_progress,  ///< Test in progress
			reserved  ///< Reserved
		};

		/// Self-test error severity
		enum class StatusSeverity {
			none,
			warning,
			error
		};

		/// Get log entry status displayable name
		static std::string get_status_displayable_name(Status s)
		{
			static const std::unordered_map<Status, std::string> m {
					{Status::unknown, "[unknown]"},
					{Status::completed_no_error, "Completed without error"},
					{Status::aborted_by_host, "Manually aborted"},
					{Status::interrupted, "Interrupted (host reset)"},
					{Status::fatal_or_unknown, "Fatal or unknown error"},
					{Status::compl_unknown_failure, "Completed with unknown failure"},
					{Status::compl_electrical_failure, "Completed with electrical failure"},
					{Status::compl_servo_failure, "Completed with servo/seek failure"},
					{Status::compl_read_failure, "Completed with read failure"},
					{Status::compl_handling_damage, "Completed: handling damage"},
					{Status::in_progress, "In progress"},
					{Status::reserved, "Unknown / reserved state"},
			};
			if (auto iter = m.find(s); iter != m.end()) {
				return iter->second;
			}
			return "[internal_error]";
		}

		/// Get severity of error status
		static StatusSeverity get_status_severity(Status s)
		{
			static const std::unordered_map<Status, StatusSeverity> m {
					{Status::unknown, StatusSeverity::none},
					{Status::completed_no_error, StatusSeverity::none},
					{Status::aborted_by_host, StatusSeverity::warning},
					{Status::interrupted, StatusSeverity::warning},
					{Status::fatal_or_unknown, StatusSeverity::error},
					{Status::compl_unknown_failure, StatusSeverity::error},
					{Status::compl_electrical_failure, StatusSeverity::error},
					{Status::compl_servo_failure, StatusSeverity::error},
					{Status::compl_read_failure, StatusSeverity::error},
					{Status::compl_handling_damage, StatusSeverity::error},
					{Status::in_progress, StatusSeverity::none},
					{Status::reserved, StatusSeverity::none},
			};
			if (auto iter = m.find(s); iter != m.end()) {
				return iter->second;
			}
			return StatusSeverity::none;
		}


		/// Get error status as a string
		std::string get_status_str() const
		{
			return (status == Status::unknown ? status_str : get_status_displayable_name(status));
		}


		/// Format lifetime hours with comma
		std::string format_lifetime_hours() const;


		uint32_t test_num = 0;  ///< Test number. always starts from 1. larger means older or newer, depending on model. 0 for capability.
		std::string type;  ///< Extended offline, Short offline, Conveyance offline, etc... . capability: unused.
		std::string status_str;  ///< Self-test routine in progress, Completed without error, etc... (as reported by log or capability)
		Status status = Status::unknown;  ///< same as status_str, but from enum
		int8_t remaining_percent = -1;  ///< Remaining %. 0% for completed, 90% for started. -1 if n/a.
		uint32_t lifetime_hours = 0;  ///< When the test happened (in lifetime hours). capability: unused.
		std::string lba_of_first_error;  ///< LBA of the first error. "-" or value (format? usually hex). capability: unused.
};


/// Output operator for debug purposes
std::ostream& operator<< (std::ostream& os, const StorageSelftestEntry& b);




/// A single parser-extracted property
class StorageProperty {
	public:

		/// Sections in output
		enum class Section {
			unknown,  ///< Used when searching in all sections
			info,  ///< Short info (--info)
			data,  ///< SMART DATA
			internal  ///< Internal application-specific data
		};

		/// Get displayable section type name
		static std::string get_section_name(Section s)
		{
			static const std::unordered_map<Section, std::string> m {
					{Section::unknown, "unknown"},
					{Section::info, "info"},
					{Section::data, "data"},
					{Section::internal, "internal"},
			};
			if (auto iter = m.find(s); iter != m.end()) {
				return iter->second;
			}
			return "[internal_error]";
		}


		/// Subsections in smart data section
		enum class SubSection {
			unknown,  ///< Used when searching in all subsections
			health,  ///< Overall-health (-H, --health)
			capabilities,  ///< General SMART Values, aka Capabilities (-c, --capabilities)
			attributes,  ///< Attributes (-A, --attributes). These need decoding.
			devstat,  ///< Device statistics (--log=devstat). These need decoding.
			error_log,  ///< Error Log (--log=error)
			selftest_log,  ///< Self-test log (--log=selftest)
			selective_selftest_log,  ///< Selective self-test log (--log=selective)
			temperature_log,  ///< SCT temperature (current and history) (--log=scttemp)
			erc_log,  ///< SCT Error Recovery Control settings (--log=scterc)
			phy_log,  ///< Phy log (--log=sataphy)
			directory_log,  ///< Directory log (--log=directory)
		};

		/// Get displayable subsection type name
		static std::string get_subsection_name(SubSection s)
		{
			static const std::unordered_map<SubSection, std::string> m {
					{SubSection::unknown, "unknown"},
					{SubSection::health, "health"},
					{SubSection::capabilities, "capabilities"},
					{SubSection::attributes, "attributes"},
					{SubSection::devstat, "devstat"},
					{SubSection::error_log, "error_log"},
					{SubSection::selftest_log, "selftest_log"},
					{SubSection::selective_selftest_log, "selective_selftest_log"},
					{SubSection::temperature_log, "temperature_log"},
					{SubSection::erc_log, "erc_log"},
					{SubSection::phy_log, "phy_log"},
					{SubSection::directory_log, "directory_log"},
			};
			if (auto iter = m.find(s); iter != m.end()) {
				return iter->second;
			}
			return "[internal_error]";
		}


		/// Get displayable value type name
		std::string get_value_type_name() const
		{
			if (std::holds_alternative<std::monostate>(value))
				return "empty";
			if (std::holds_alternative<std::string>(value))
				return "string";
			if (std::holds_alternative<int64_t>(value))
				return "integer";
			if (std::holds_alternative<bool>(value))
				return "bool";
			if (std::holds_alternative<std::chrono::seconds>(value))
				return "time_length";
			if (std::holds_alternative<StorageCapability>(value))
				return "capability";
			if (std::holds_alternative<StorageAttribute>(value))
				return "attribute";
			if (std::holds_alternative<StorageStatistic>(value))
				return "statistic";
			if (std::holds_alternative<StorageErrorBlock>(value))
				return "error_block";
			if (std::holds_alternative<StorageSelftestEntry>(value))
				return "selftest_entry";
			return "[internal_error]";
		}


		/// Check if this is an empty object with no value set.
		bool empty() const
		{
			return std::holds_alternative<std::monostate>(value);
		}


		/// Dump the property to a stream for debugging purposes
		void dump(std::ostream& os, std::size_t internal_offset = 0) const;


		/// Format this property for debugging purposes
		std::string format_value(bool add_reported_too = false) const;


		/// Get value of type T
		template<typename T>
		const T& get_value() const
		{
			return std::get<T>(value);
		}


		/// Check if value is of type T
		template<typename T>
		bool is_value_type() const
		{
			return std::holds_alternative<T>(value);
		}


		/// Get property description (used in tooltips)
		std::string get_description(bool clean = false) const
		{
			if (clean)
				return this->description;
			return (this->description.empty() ? "No description available" : this->description);
		}


		/// Set property description (used in tooltips)
		void set_description(const std::string& descr)
		{
			this->description = descr;
		}


		/// Set smartctl-reported name, generic (internal) name, readable name
		void set_name(const std::string& rep_name, const std::string& gen_name = "", const std::string& read_name = "")
		{
			this->reported_name = rep_name;
			this->generic_name = (gen_name.empty() ? this->reported_name : gen_name);
			this->displayable_name = (read_name.empty() ? this->reported_name : read_name);
		}


		std::string reported_name;  ///< Property name as reported by smartctl.
		std::string generic_name;  ///< Generic (internal) name. May be same as reported_name, or something more program-identifiable.
		std::string displayable_name;  ///< Readable property name. May be same as reported_name, or something more user-readable. Possibly translatable.

		std::string description;  ///< Property description (for tooltips, etc...)

		Section section = Section::unknown;  ///< Section this property belongs to
		SubSection subsection = SubSection::unknown;  ///< Subsection this property belongs to

		std::string reported_value;  ///< String representation of the value as reported
		std::string readable_value;  ///< User-friendly readable representation of value. if empty, use the other members.

		std::variant<
			std::monostate,  ///< None
			std::string,  ///< Value (if it's a string)
			int64_t,   ///< Value (if it's an integer)
			bool,  ///< Value (if it's bool)
			std::chrono::seconds,  ///< Value in seconds (if it's time interval)
			StorageCapability,  ///< Value (if it's a capability)
			StorageAttribute,  ///< Value (if it's an attribute)
			StorageStatistic,  ///< Value (if it's a statistic from devstat)
			StorageErrorBlock,  ///< Value (if it's a error block)
			StorageSelftestEntry  ///< Value (if it's a self-test entry)
		> value;

		WarningLevel warning = WarningLevel::none;  ///< Warning severity for this property
		std::string warning_reason;  // Warning reason (displayable)

		bool show_in_ui = true;  ///< Whether to show this property in UI or not

};




/// Output operator for debug purposes
inline std::ostream& operator<< (std::ostream& os, const StorageProperty& p)
{
	p.dump(os);
	return os;
}






#endif

/// @}
