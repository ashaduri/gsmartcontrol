/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
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

#include "hz/optional_value.h"
#include "hz/cstdint.h"




/// Holds one block of "capabilities" subsection
/// (only for non-time-interval blocks).
class StorageCapability {
	public:

		/// Constructor
		StorageCapability() : flag_value(0x0)
		{ }

		/// String list
		typedef std::vector<std::string> strvalue_list_t;

		std::string reported_flag_value;  ///< original flag value as a string
		uint16_t flag_value;  ///< Flag value. This is one or sometimes two bytes (maybe more?)
		std::string reported_strvalue;  ///< Original flag descriptions
		strvalue_list_t strvalues;  ///< A list of capabilities in the block.
};


/// Output operator for debug purposes
std::ostream& operator<< (std::ostream& os, const StorageCapability& p);





/// Holds one line of "attributes" subsection
class StorageAttribute {
	public:

		/// Disk type the attribute may match
		enum DiskType {
			DiskAny,  ///< Any disk type
			DiskHDD,  ///< HDD (rotational) only
			DiskSSD  ///< SSD only
		};

		/// Attribute pre-failure / old-age type
		enum attr_t {
			attr_type_unknown,  ///< Unknown
			attr_type_prefail,  ///< Pre-failure (reported: Pre-fail)
			attr_type_oldage  ///< Old age (reported: Old_age)
		};

		/// Get readable attribute type name
		static std::string get_attr_type_name(attr_t a)
		{
			switch (a) {
				case attr_type_unknown: return "[unknown]";
				case attr_type_prefail: return "pre-failure";
				case attr_type_oldage: return "old age";
			};
			return "[error]";
		}


		/// Attribute when-updated type
		enum update_t {
			update_type_unknown,  ///< Unknown
			update_type_always,  ///< Continuously (reported: Always)
			update_type_offline  ///< Only during offline data collection (reported: Offline)
		};

		/// Get readable when-updated type name
		static std::string get_update_type_name(update_t a)
		{
			switch (a) {
				case update_type_unknown: return "[unknown]";
				case update_type_always: return "continuously";
				case update_type_offline: return "on offline data collect.";
			};
			return "[error]";
		}


		/// Attribute when-failed type
		enum fail_time_t {
			fail_time_unknown,  ///< Unknown
			fail_time_none,  ///< Never (reported: -)
			fail_time_past,  ///< In the past (reported: In_the_past)
			fail_time_now  ///< Now (reported: FAILING_NOW)
		};

		/// Get a readable when-failed type name
		static std::string get_fail_time_name(fail_time_t a)
		{
			switch (a) {
				case fail_time_unknown: return "[unknown]";
				case fail_time_none: return "never";
				case fail_time_past: return "in the past";
				case fail_time_now: return "now";
			};
			return "[error]";
		}


		/// Constructor
		StorageAttribute() : id(-1), attr_type(attr_type_unknown), update_type(update_type_unknown),
			when_failed(fail_time_unknown), raw_value_int(0)
		{ }


		int32_t id;  ///< Attribute ID (most vendors agree on this)
		std::string flag;  ///< Some have it in 0xXXXX format, others in "PO--C-" format (some WD drives?).
		hz::OptionalValue<uint8_t> value;  ///< Normalized value. May be unset ("---").
		hz::OptionalValue<uint8_t> worst;  ///< Worst ever value. May be unset ("---").
		hz::OptionalValue<uint8_t> threshold;  ///< Threshold for normalized value. May be unset ("---").
		attr_t attr_type;  ///< Attribute pre-fail / old-age type
		update_t update_type;  ///< When-updated type
		fail_time_t when_failed;  ///< When-failed type
		std::string raw_value;  ///< Raw value as a string, as presented by smartctl (formatted).
		int64_t raw_value_int;  ///< Same as raw_value, but parsed as int64. original value is 6 bytes I think.

};


/// Output operator for debug purposes
std::ostream& operator<< (std::ostream& os, const StorageAttribute& p);




/// Holds one error block of "error log" subsection
class StorageErrorBlock {
	public:

		/// Constructor
		StorageErrorBlock() : error_num(0), lifetime_hours(0)
		{ }

		/// Get readable error types from reported types
		static std::string get_readable_error_types(const std::vector<std::string>& types);

		uint32_t error_num;  ///< Error number
		uint32_t lifetime_hours;  ///< When the error occurred (in lifetime hours)
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
		enum status_t {
			status_unknown,  ///< Initial state
			status_completed_no_error,  ///< Completed with no error, or no test was run
			status_aborted_by_host,  ///< Aborted by host
			status_interrupted,  ///< Interrupted by user
			status_fatal_or_unknown,  ///< Fatal or unknown error. Treated as test failure.
			status_compl_unknown_failure,  ///< Completed with unknown error. Treated as test failure.
			status_compl_electrical_failure,  ///< Completed with electrical error. Treated as test failure.
			status_compl_servo_failure,  ///< Completed with servo error. Treated as test failure.
			status_compl_read_failure,  ///< Completed with read error. Treated as test failure.
			status_compl_handling_damage,  ///< Completed with handling damage error. Treated as test failure.
			status_in_progress,  ///< Test in progress
			status_reserved  ///< Reserved
		};

		/// Self-test error severity
		enum status_severity_t {
			severity_none,
			severity_warn,
			severity_error
		};

		/// Get log entry status displayable name
		static std::string get_status_name(status_t s)
		{
			switch (s) {
				case status_unknown: return "[unknown]";
				case status_completed_no_error: return "Completed without error";
				case status_aborted_by_host: return "Manually aborted";
				case status_interrupted: return "Interrupted (host reset)";
				case status_fatal_or_unknown: return "Fatal or unknown error";
				case status_compl_unknown_failure: return "Completed with unknown failure";
				case status_compl_electrical_failure: return "Completed with electrical failure";
				case status_compl_servo_failure: return "Completed with servo/seek failure";
				case status_compl_read_failure: return "Completed with read failure";
				case status_compl_handling_damage: return "Completed: handling damage";
				case status_in_progress: return "In progress";
				case status_reserved: return "Unknown / reserved state";
			};
			return "[error]";
		}

		/// Get severity of error status
		static status_severity_t get_status_severity(status_t s)
		{
			switch (s) {
				case status_unknown: return severity_none;
				case status_completed_no_error: return severity_none;
				case status_aborted_by_host: return severity_warn;
				case status_interrupted: return severity_warn;
				case status_fatal_or_unknown: return severity_error;
				case status_compl_unknown_failure: return severity_error;
				case status_compl_electrical_failure: return severity_error;
				case status_compl_servo_failure: return severity_error;
				case status_compl_read_failure: return severity_error;
				case status_compl_handling_damage: return severity_error;
				case status_in_progress: return severity_none;
				case status_reserved: return severity_none;
			};
			return severity_none;
		}

		/// Constructor
		StorageSelftestEntry() : test_num(0), status(status_unknown),
				remaining_percent(-1), lifetime_hours()
		{ }

		/// Get error status as a string
		std::string get_status_str() const
		{
			return (status == status_unknown ? status_str : get_status_name(status));
		}


		uint32_t test_num;  ///< Test number. always starts from 1. larger means older or newer, depending on model. 0 for capability.
		std::string type;  ///< Extended offline, Short offline, Conveyance offline, etc... . capability: unused.
		std::string status_str;  ///< Self-test routine in progress, Completed without error, etc... (as reported by log or capability)
		status_t status;  ///< same as status_str, but from enum
		int8_t remaining_percent;  ///< Remaining %. 0% for completed, 90% for started. -1 if n/a.
		uint32_t lifetime_hours;  ///< When the test happened (in lifetime hours). capability: unused.
		std::string lba_of_first_error;  ///< LBA of the first error. "-" or value (format?). capability: unused.
};


/// Output operator for debug purposes
std::ostream& operator<< (std::ostream& os, const StorageSelftestEntry& b);




/// A single parser-extracted property
class StorageProperty {
	public:

		/// Value types
		enum value_type_t {
			value_type_unknown,  ///< Empty value
			value_type_string,  ///< String value
			value_type_integer,  ///< int64
			value_type_bool,  ///< Enabled / disabled; available / not available; passed / not passed.
			value_type_time_length,  ///< Time length in seconds.

			value_type_capability,  ///< For "capabilities" subsection (non-time-interval blocks only)
			value_type_attribute,  ///< For "attributes" subsection
			value_type_error_block,  // For "error_log" subsection
			value_type_selftest_entry  // For "selftest_log" subsection
		};

		/// Get displayable value type name
		static std::string get_value_type_name(value_type_t type)
		{
			switch(type) {
				case value_type_unknown: return "empty";
				case value_type_string: return "string";
				case value_type_integer: return "integer";
				case value_type_bool: return "bool";
				case value_type_time_length: return "time_length";
				case value_type_capability: return "capability";
				case value_type_attribute: return "attribute";
				case value_type_error_block: return "error_block";
				case value_type_selftest_entry: return "selftest_entry";
			}
			return "[error]";
		}


		/// Sections in output
		enum section_t {
			section_unknown,  ///< Used when searching in all sections
			section_info,  ///< Short info (--info)
			section_data,  ///< SMART DATA
			section_internal  ///< Internal application-specific data
		};

		/// Get displayable section type name
		static std::string get_section_name(section_t s)
		{
			switch(s) {
				case section_unknown: return "unknown";
				case section_info: return "info";
				case section_data: return "data";
				case section_internal: return "internal";
			}
			return "[error]";
		}


		/// Subsections in smart data section
		enum subsection_t {
			subsection_unknown,  ///< Used when searching in all subsections
			subsection_health,  ///< Overall-health (-H, --health)
			subsection_capabilities,  ///< General SMART Values, aka Capabilities (-c, --capabilities)
			subsection_attributes,  ///< Attributes (-A, --attributes). These need decoding.
			subsection_error_log,  ///< Error Log (-l error)
			subsection_selftest_log,  ///< Self-test log (-l selftest)
			subsection_selective_selftest_log  ///< Selective self-test log and settings
		};

		/// Get displayable subsection type name
		static std::string get_subsection_name(subsection_t s)
		{
			switch(s) {
				case subsection_unknown: return "unknown";
				case subsection_health: return "health";
				case subsection_capabilities: return "capabilities";
				case subsection_attributes: return "attributes";
				case subsection_error_log: return "error_log";
				case subsection_selftest_log: return "selftest_log";
				case subsection_selective_selftest_log: return "selective_selftest_log";
			}
			return "[error]";
		}


		/// Warning type
		enum warning_t {
			warning_none,  ///< No warning
			warning_notice,  ///< A known attribute is somewhat disturbing, but no smart error
			warning_warn,  ///< SMART warning is raised by old-age attribute
			warning_alert  ///< SMART warning is raised by pre-fail attribute, and similar errors
		};


		/// Constructor
		StorageProperty()
			: section(section_unknown), subsection(subsection_unknown),
			value_type(value_type_unknown), warning(warning_none), show_in_ui(true)
		{
// 			value_from_db = false;
			value_integer = 0;  // this should nullify all union members
		}


		/// Check if this is an empty object with no value set.
		bool empty() const
		{
			return (value_type == value_type_unknown);
		}


		/// Dump the property to a stream for debugging purposes
		void dump(std::ostream& os, int internal_offset = 0) const;


		/// Format this property for debugging purposes
		std::string format_value(bool add_reported_too = false) const;


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
			this->readable_name = (read_name.empty() ? this->reported_name : read_name);
		}



		std::string reported_name;  ///< Property name as reported by smartctl.
		std::string generic_name;  ///< Generic (internal) name. May be same as reported_name, or something more program-identifiable.
		std::string readable_name;  ///< Readable property name. May be same as reported_name, or something more user-readable. Possibly translatable.

		std::string description;  ///< Property description (for tooltips, etc...)

		section_t section;  ///< Section this property belongs to
		subsection_t subsection;  ///< Subsection this property belongs to

// 		bool empty_value;  // the property has an empty value
// 		bool value_from_db;  // value retrieved from the database, not the drive

		std::string reported_value;  ///< String representation of the value as reported
		std::string readable_value;  ///< User-friendly readable representation of value. if empty, use the other members.

		value_type_t value_type;  ///< Property value type

		std::string value_string;  ///< Value (if it's a string)
		std::string value_version;  ///< Value (if it's a version)
		union {
			int64_t value_integer;  ///< Value (if it's an integer)
			bool value_bool;  ///< Value (if it's bool)
			uint64_t value_time_length;  ///< Value in seconds (if it's time interval)
		};

		StorageCapability value_capability;  ///< Value (if it's a capability)
		StorageAttribute value_attribute;  ///< Value (if it's an attribute)
		StorageErrorBlock value_error_block;  ///< Value (if it's a error block)
		StorageSelftestEntry value_selftest_entry;  ///< Value (if it's a self-test entry)

		warning_t warning;  ///< Warning severity for this property
		std::string warning_reason;  // Warning reason (displayable)

		bool show_in_ui;  ///< Whether to show this property in UI or not

};




/// Output operator for debug purposes
inline std::ostream& operator<< (std::ostream& os, const StorageProperty& p)
{
	p.dump(os);
	return os;
}






#endif

/// @}
