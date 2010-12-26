/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef SMARTCTL_PARSER_H
#define SMARTCTL_PARSER_H

#include <string>
#include <vector>

#include "storage_property.h"



class SmartctlParser {

	public:

		typedef std::vector<StorageProperty> prop_list_t;

		// Note: ALL parse_* functions (except parse_full() and parse_version())
		// expect data in unix-newline format!


		// Parse full "smartctl -a" output
		bool parse_full(const std::string& s);


		// supply any output of smartctl here.
		static bool parse_version(const std::string& s, std::string& version, std::string& version_full);


		// check that the version of smartctl output can be parsed with this parser.
		static bool check_version(const std::string& version_str, const std::string& version_full_str);


		// convert e.g. "1,000,204,886,016 bytes" to 1.00 TiB [931.51 GB, 1000204886016 bytes]
		static std::string parse_byte_size(const std::string& str, uint64_t& bytes, bool extended);


		// You don't really need to call these functions, use the ones above.


		// Parse the section part (with "=== .... ===" header) - info or data sections.
		bool parse_section(const std::string& header, const std::string& body);


		// Parse the info section (without "===" header)
		bool parse_section_info(const std::string& body);

		// Parse a component (one line) of the info section
		bool parse_section_info_property(StorageProperty& p);


		// Parse the Data section (without "===" header)
		bool parse_section_data(const std::string& body);

		// Parse subsections of Data section
		bool parse_section_data_subsection_health(const std::string& sub);
		bool parse_section_data_subsection_capabilities(const std::string& sub);
		bool parse_section_data_subsection_attributes(const std::string& sub);
		bool parse_section_data_subsection_error_log(const std::string& sub);
		bool parse_section_data_subsection_selftest_log(const std::string& sub);
		bool parse_section_data_subsection_selective_selftest_log(const std::string& sub);

		// Check the capabilities for internal properties we can use.
		bool parse_section_data_internal_capabilities(StorageProperty& cap);




		void clear()
		{
			data_full_.clear();
			data_section_info_.clear();
			data_section_data_.clear();
			error_msg_.clear();

			properties_.clear();
		}


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
		std::string get_error_msg() const
		{
			return "Cannot parse smartctl output: " + error_msg_;
		}


		const prop_list_t& get_properties() const
		{
			return properties_;
		}



	private:


		// adds a property into property list, looks up and sets its description
		void add_property(StorageProperty p);


		void set_data_full(const std::string& s)
		{
			data_full_ = s;
		}

		void set_data_section_info(const std::string& s)
		{
			data_section_info_ = s;
		}

		void set_data_section_data(const std::string& s)
		{
			data_section_data_ = s;
		}


		void set_error_msg(const std::string& s)
		{
			error_msg_ = s;
		}



		prop_list_t properties_;

		// These are filled by the appropriate parse_* functions
		std::string data_full_;
		std::string data_section_info_;
		std::string data_section_data_;

		std::string error_msg_;  // on error this will be filled with displayable message


};












#endif
