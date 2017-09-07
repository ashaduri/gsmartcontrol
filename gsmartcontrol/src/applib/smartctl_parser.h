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

#ifndef SMARTCTL_PARSER_H
#define SMARTCTL_PARSER_H

#include <string>
#include <vector>

#include "storage_property.h"



/// Smartctl parser.
/// Note: ALL parse_* functions (except parse_full() and parse_version())
/// expect data in unix-newline format!
class SmartctlParser {

	public:

		/// Property list
		typedef std::vector<StorageProperty> prop_list_t;


		/// Constructor
		SmartctlParser();


		/// Parse full "smartctl -x" output
		bool parse_full(const std::string& s, StorageAttribute::DiskType disk_type);


		/// Supply any output of smartctl here, the smartctl version will be retrieved.
		static bool parse_version(const std::string& s, std::string& version, std::string& version_full);


		/// Check that the version of smartctl output can be parsed with this parser.
		static bool check_parsed_version(const std::string& version_str, const std::string& version_full_str);


		/// Convert e.g. "1,000,204,886,016 bytes" to 1.00 TiB [931.51 GB, 1000204886016 bytes]
		static std::string parse_byte_size(const std::string& str, uint64_t& bytes, bool extended);


		// You don't really need to call these functions, use the ones above.


		/// Parse the section part (with "=== .... ===" header) - info or data sections.
		bool parse_section(const std::string& header, const std::string& body);


		/// Parse the info section (without "===" header).
		/// This includes --info and --get=all.
		bool parse_section_info(const std::string& body);

		/// Parse a component (one line) of the info section
		bool parse_section_info_property(StorageProperty& p);


		/// Parse the Data section (without "===" header)
		bool parse_section_data(const std::string& body);

		/// Parse subsections of Data section
		bool parse_section_data_subsection_health(const std::string& sub);
		bool parse_section_data_subsection_capabilities(const std::string& sub);
		bool parse_section_data_subsection_attributes(const std::string& sub);
		bool parse_section_data_subsection_directory_log(const std::string& sub);
		bool parse_section_data_subsection_error_log(const std::string& sub);
		bool parse_section_data_subsection_selftest_log(const std::string& sub);
		bool parse_section_data_subsection_selective_selftest_log(const std::string& sub);
		bool parse_section_data_subsection_scttemp_log(const std::string& sub);
		bool parse_section_data_subsection_scterc_log(const std::string& sub);
		bool parse_section_data_subsection_devstat(const std::string& sub);
		bool parse_section_data_subsection_sataphy(const std::string& sub);

		/// Check the capabilities for internal properties we can use.
		bool parse_section_data_internal_capabilities(StorageProperty& cap);


		/// Clear parsed data
		void clear()
		{
			data_full_.clear();
			data_section_info_.clear();
			data_section_data_.clear();
			error_msg_.clear();

			properties_.clear();
		}


		/// Get "full" data, as passed to parse_full().
		std::string get_data_full() const
		{
			return data_full_;
		}

/*
		std::string get_data_section_info() const
		{
			return data_section_info_;
		}

		std::string get_data_section_data() const
		{
			return data_section_data_;
		}
*/

		/// Get parse error message. Call this only if parsing doesn't succeed,
		/// to get a friendly error message.
		std::string get_error_msg() const
		{
			return "Cannot parse smartctl output: " + error_msg_;
		}


		/// Get parse result properties
		const prop_list_t& get_properties() const
		{
			return properties_;
		}



	private:


		/// Add a property into property list, look up and set its description
		void add_property(StorageProperty p);


		/// Set "full" data ("smartctl -x" output)
		void set_data_full(const std::string& s)
		{
			data_full_ = s;
		}


		/// Set "info" section data ("smartctl -i" output, or the first part of "smartctl -x" output)
		void set_data_section_info(const std::string& s)
		{
			data_section_info_ = s;
		}


		/// Parse "data" section data (the second part of "smartctl -x" output).
		void set_data_section_data(const std::string& s)
		{
			data_section_data_ = s;
		}


		/// Set error message
		void set_error_msg(const std::string& s)
		{
			error_msg_ = s;
		}



		prop_list_t properties_;  ///< Parsed data properties

		std::string data_full_;  ///< full data, filled by parse_full()
		std::string data_section_info_;  ///< "info" section data, filled by parse_section_info()
		std::string data_section_data_;  ///< "data" section data, filled by parse_section_data()

		std::string error_msg_;  ///< This will be filled with some displayable message on error

		StorageAttribute::DiskType disk_type_;  ///< Disk type (HDD, SSD)

};






#endif

/// @}
