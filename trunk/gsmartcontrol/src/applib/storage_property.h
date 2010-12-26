/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef STORAGE_PROPERTY_H
#define STORAGE_PROPERTY_H

#include <string>
#include <vector>
#include <iosfwd>

#include "hz/cstdint.h"




// holds one block of "capabilities" subsection
// (only for non-time-interval blocks).
class StorageCapability {
	public:

		StorageCapability() : flag_value(0x0)
		{ }

		typedef std::vector<std::string> strvalue_list_t;

		std::string reported_flag_value;  // original flag value
		uint16_t flag_value;  // flag value. this is one or sometimes two bytes (maybe more?)
		std::string reported_strvalue;  // original flag descriptions
		strvalue_list_t strvalues;
};


std::ostream& operator<< (std::ostream& os, const StorageCapability& p);





// holds one line of "attributes" subsection
class StorageAttribute {
	public:

		enum attr_t {
			attr_type_unknown,
			attr_type_prefail,  // reported: Pre-fail
			attr_type_oldage  // reported: Old_age
		};

		static std::string get_attr_type_name(attr_t a)
		{
			switch (a) {
				case attr_type_unknown: return "[unknown]";
				case attr_type_prefail: return "pre-failure";
				case attr_type_oldage: return "old age";
			};
			return "[error]";
		}


		enum update_t {
			update_type_unknown,
			update_type_always,  // reported: Always
			update_type_offline  // reported: Offline
		};

		static std::string get_update_type_name(update_t a)
		{
			switch (a) {
				case update_type_unknown: return "[unknown]";
				case update_type_always: return "continuously";
				case update_type_offline: return "on offline data collect.";
			};
			return "[error]";
		}


		enum fail_time_t {
			fail_time_unknown,
			fail_time_none,  // reported: -
			fail_time_past,  // reported: In_the_past
			fail_time_now  // reported: FAILING_NOW
		};

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


		StorageAttribute() : id(-1), value(0), worst(0), threshold(0),
			attr_type(attr_type_unknown), update_type(update_type_unknown),
			when_failed(fail_time_unknown), raw_value_int(0)
		{ }


		int32_t id;
		std::string flag;  // some have it in 0xXXXX format, others in "PO--C-" format (some WD drives?).
		uint8_t value;
		uint8_t worst;
		uint8_t threshold;
		attr_t attr_type;
		update_t update_type;
		fail_time_t when_failed;
		std::string raw_value;  // as presented by smartctl (formatted).
		int64_t raw_value_int;  // same as raw_value, but parsed as int. original value is 6 bytes I think.

};


std::ostream& operator<< (std::ostream& os, const StorageAttribute& p);




// holds one error block of "error log" subsection
class StorageErrorBlock {
	public:

		StorageErrorBlock() : error_num(0), lifetime_hours(0)
		{ }

		static std::string get_readable_error_types(const std::vector<std::string>& types);

		uint32_t error_num;  // error number
		uint32_t lifetime_hours;  // when it occurred
		std::string device_state;  // "active or idle", standby, etc...
		std::vector<std::string> reported_types;  // e.g. array of reported strings, e.g. "UNC".
		std::string type_more_info;  // more info on error type (e.g. "at LBA = 0x0253eac0 = 39054016")
};


std::ostream& operator<< (std::ostream& os, const StorageErrorBlock& b);




// Holds one entry of selftest_log subsection.
// Also, holds "Self-test execution status" capability's "internal" section version.
class StorageSelftestEntry {
	public:

		enum status_t {
			status_unknown,  // initial state
			status_completed_no_error,
			status_aborted_by_host,
			status_interrupted,
			status_fatal_or_unknown,  // error
			status_compl_unknown_failure,  // error
			status_compl_electrical_failure,  // error
			status_compl_servo_failure,  // error
			status_compl_read_failure,  // error
			status_compl_handling_damage,  // error
			status_in_progress,
			status_reserved
		};

		enum status_severity_t {
			severity_none,
			severity_warn,
			severity_error
		};

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

		StorageSelftestEntry() : test_num(0), status(status_unknown),
				remaining_percent(-1), lifetime_hours()
		{ }

		std::string get_status_str() const
		{
			return (status == status_unknown ? status_str : get_status_name(status));
		}

		// test number. always starts from 1. larger means older or newer, depending on model.
		uint32_t test_num;  // 0 for capability.
		std::string type;  // Extended offline, Short offline, Conveyance offline, etc... . capability: unused.
		std::string status_str;  // Self-test routine in progress, Completed without error, etc... (as reported by log or capability)
		status_t status;  // same as above, but from enum
		int8_t remaining_percent;  // %. 0% for completed, 90% for started. -1 if n/a.
		uint32_t lifetime_hours;  // when it happened. capability: unused.
		std::string lba_of_first_error;  // "-" or value (format?). capability: unused.
};


std::ostream& operator<< (std::ostream& os, const StorageSelftestEntry& b);




class StorageProperty {

	public:

		// value types
		enum value_type_t {
			value_type_unknown,  // empty value
			value_type_string,  // string value
			value_type_integer,  // int64
			value_type_bool,  // enabled / disabled, available / not available. passed / not passed.
			value_type_time_length,  // in seconds. e.g. 300 seconds.

			value_type_capability,  // for "capabilities" subsection (non-time-interval blocks only)
			value_type_attribute,  // for "attributes" subsection
			value_type_error_block,  // for "error_log" subsection
			value_type_selftest_entry  // for "selftest_log" subsection
		};

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


		// sections in output
		enum section_t {
			section_unknown,  // use when searching in all sections
			section_info,  // Short info (--info)
			section_data,  // SMART DATA
			section_internal  // internal application-specific data
		};

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


		// subsections in smart data section
		enum subsection_t {
			subsection_unknown,  // use when searching in all subsections
			subsection_health,  // overall-health (-H, --health)
			subsection_capabilities,  // General SMART Values, aka Capabilities (-c, --capabilities)
			subsection_attributes,  // Attributes (-A, --attributes). These need decoding.
			subsection_error_log,  // Error Log (-l error)
			subsection_selftest_log,  // Self-test log (-l selftest)
			subsection_selective_selftest_log  // Selective self-test log and settings
		};

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


		enum warning_t {
			warning_none,
			warning_notice,  // a known attribute is somewhat disturbing, but no smart error
			warning_warn,  // smart warning is raised by old-age attribute
			warning_alert  // smart warning is raised by pre-fail attribute, and similar errors
		};



		StorageProperty()
			: section(section_unknown), subsection(subsection_unknown),
			value_type(value_type_unknown), warning(warning_none)
		{
// 			value_from_db = false;
			value_integer = 0;  // this should nullify all union members
		}


		bool empty() const  // empty object - no value set.
		{
			return (value_type == value_type_unknown);
		}


		void dump(std::ostream& os, int internal_offset = 0) const;


		std::string format_value(bool add_reported_too = false) const;



		std::string get_description(bool clean = false) const
		{
			if (clean)
				return this->description;
			return (this->description.empty() ? "No description available" : this->description);
		}


		void set_description(const std::string& descr)
		{
			this->description = descr;
		}


		void set_name(const std::string& rep_name, const std::string& gen_name = "", const std::string& read_name = "")
		{
			this->reported_name = rep_name;
			this->generic_name = (gen_name.empty() ? this->reported_name : gen_name);
			this->readable_name = (read_name.empty() ? this->reported_name : read_name);
		}



		// property name as reported by smartctl.
		std::string reported_name;
		// property name. may be same as reported_name, or something more program-identifiable.
		std::string generic_name;
		// property name. may be same as reported_name, or something more user-readable. possibly translatable.
		std::string readable_name;

		std::string description;  // description. for tooltips, etc...

		section_t section;
		subsection_t subsection;

// 		bool empty_value;  // the property has an empty value
// 		bool value_from_db;  // value retrieved from the database, not the drive

		std::string reported_value;  // string representation of the value as reported
		std::string readable_value;  // user-friendly readable representation of value. if empty, use others.

		value_type_t value_type;

		std::string value_string;
		std::string value_version;
		union {
			int64_t value_integer;
			bool value_bool;
			uint64_t value_time_length;  // always in seconds
		};

		StorageCapability value_capability;
		StorageAttribute value_attribute;
		StorageErrorBlock value_error_block;
		StorageSelftestEntry value_selftest_entry;

		warning_t warning;  // warning severity
		std::string warning_reason;  // reason (displayable)

};




inline std::ostream& operator<< (std::ostream& os, const StorageProperty& p)
{
	p.dump(os);
	return os;
}






#endif
