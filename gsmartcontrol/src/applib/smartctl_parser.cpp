/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <clocale>  // localeconv

#include "hz/locale_tools.h"  // ScopedCLocale, locale_c_get().
#include "hz/cstdint.h"  // uint64_t
#include "hz/string_algo.h"  // string_*
#include "hz/string_num.h"  // string_is_numeric, number_to_string
#include "hz/format_unit.h"  // format_size
#include "hz/debug.h"  // debug_*

#include "app_pcrecpp.h"
#include "smartctl_parser.h"
#include "storage_property_descr.h"
#include "storage_property_colors.h"


namespace {


	inline StorageProperty app_get_checksum_error_property(const std::string& name)
	{
		StorageProperty p;
		p.section = StorageProperty::section_data;

		if (name == "Attribute Data") {
			p.subsection = StorageProperty::subsection_attributes;
			p.set_name(name, "attribute_data_checksum_error");

		} else if (name == "Attribute Thresholds") {
			p.subsection = StorageProperty::subsection_attributes;
			p.set_name(name, "attribute_thresholds_checksum_error");

		} else if (name == "ATA Error Log") {
			p.subsection = StorageProperty::subsection_error_log;
			p.set_name(name, "ata_error_log_checksum_error");

		} else if (name == "Self-Test Log") {
			p.subsection = StorageProperty::subsection_selftest_log;
			p.set_name(name, "selftest_log_checksum_error");
		}

		p.readable_name = "Error in " + name + " structure";

		p.reported_value = "checksum error";
		p.value_type = StorageProperty::value_type_string;
		p.value_string = p.reported_value;

		return p;
	}


}





// Parse full "smartctl -a" output
bool SmartctlParser::parse_full(const std::string& full)
{
	this->clear();  // clear previous data

	this->set_data_full(full);


	// -------------------- Fix the output so it doesn't interfere with proper parsing

	// perform any2unix
	std::string s = hz::string_trim_copy(hz::string_any_to_unix_copy(full));

	if (s.empty()) {
		set_error_msg("Smartctl data is empty.");
		debug_out_warn("app", DBG_FUNC_MSG << "Empty string passed as an argument. Returning.\n");
		return false;
	}


	// The first line may be a command, filter it out. e.g.
	// # smartctl -a /dev/sda
	// NO NEED: We ignore everything non-section (except version info).
	// Note: We ignore non-section lines, so we don't need any filtering here.
// 	{
// 		app_pcre_replace_once("/^# .*$/", "", s);  // replace first only, on the first line only.
// 	}


	// Checksum warnings are kind of randomly distributed, so
	// extract and remove them.
	{
		pcrecpp::RE re = app_pcre_re("/\\nWarning! SMART (.+) Structure error: invalid SMART checksum\\.$/mi");

		std::string name;
		pcrecpp::StringPiece input(s);  // position tracker

		while (re.FindAndConsume(&input, &name)) {
			add_property(app_get_checksum_error_property(hz::string_trim_copy(name)));
		}

		app_pcre_replace(re, "", s);  // remove them from s.
	}

	// Remove some additional stuff which doesn't fit
	// Display this warning somewhere? (info section?)
	// Or not, these options don't do anything crucial - just some translation stuff.
	{
		app_pcre_replace("/\\n.*May need -F samsung or -F samsung2 enabled; see manual for details\\.$/mi",
				"", s);  // remove from s
	}


	// The Warning: parts also screw up newlines sometimes (making double-newlines,
	// confusing for section separation).
	{
		pcrecpp::RE re = app_pcre_re("/^(Warning: ATA error count.*\\n)\\n/mi");

		std::string match;
		if (app_pcre_match(re, s, &match)) {
			app_pcre_replace(re, match, s);  // make one newline less
		}
	}


	// If the device doesn't support many things, the warnings aren't separated (for sections).
	// Fix that.
	{
		pcrecpp::RE re1 = app_pcre_re("/^(Warning: device does not support Error Logging)$/mi");
		pcrecpp::RE re2 = app_pcre_re("/^(Warning: device does not support Self Test Logging)$/mi");
		pcrecpp::RE re3 = app_pcre_re("/^(Device does not support Selective Self Tests\\/Logging)$/mi");
		std::string match;

		if (app_pcre_match(re1, s, &match))
			app_pcre_replace(re1, "\n" + match, s);  // add an extra newline

		if (app_pcre_match(re2, s, &match))
			app_pcre_replace(re2, "\n" + match, s);  // add an extra newline

		if (app_pcre_match(re3, s, &match))
			app_pcre_replace(re3, "\n" + match, s);  // add an extra newline

	}


	// ------------------- Parsing

	// version info

	std::string version, version_full;
	if (!parse_version(s, version, version_full)) {
		set_error_msg("Cannot extract smartctl version information.");
		debug_out_warn("app", DBG_FUNC_MSG << "Cannot extract version information. Returning.\n");
		return false;

	} else {
		{
			StorageProperty p;
			p.set_name("Smartctl version", "smartctl_version");
			p.reported_value = version;
			p.value_type = StorageProperty::value_type_string;
			p.value_string = p.reported_value;
			p.section = StorageProperty::section_info;  // add to info section
			add_property(p);
		}
		{
			StorageProperty p;
			p.set_name("Smartctl version", "smartctl_version_full");
			p.reported_value = version_full;
			p.value_type = StorageProperty::value_type_string;
			p.value_string = p.reported_value;
			p.section = StorageProperty::section_info;  // add to info section
			add_property(p);
		}
	}

	if (!check_version(version, version_full)) {
		set_error_msg("Incompatible smartctl version.");
		debug_out_warn("app", DBG_FUNC_MSG << "Incompatible smartctl version. Returning.\n");
		return false;
	}


	// sections

	std::string::size_type section_start_pos = 0, section_end_pos = 0, tmp_pos = 0;
	bool status = false;  // true if at least one section was parsed

	// sections are started by
	// === START OF <NAME> SECTION ===
	while (section_start_pos != std::string::npos
			&& (section_start_pos = s.find("===", section_start_pos)) != std::string::npos) {

		tmp_pos = s.find("\n", section_start_pos);  // works with \r\n too.

		// trim is needed to remove potential \r in the end
		std::string section_header = hz::string_trim_copy(s.substr(section_start_pos,
				(tmp_pos == std::string::npos ? tmp_pos : (tmp_pos - section_start_pos)) ));

		if (tmp_pos != std::string::npos)
			++tmp_pos;  // set to start of the next section

		section_end_pos = s.find("===", tmp_pos);  // start of the next section
		std::string section_body_str = hz::string_trim_copy(s.substr(tmp_pos,
				(section_end_pos == std::string::npos ? section_end_pos : section_end_pos - tmp_pos)));

		status = parse_section(section_header, section_body_str) || status;
		section_start_pos = section_end_pos;
	}

	if (!status) {
		set_error_msg("No ATA sections could be parsed.");
		debug_out_warn("app", DBG_FUNC_MSG << "No ATA sections could be parsed. Returning.\n");
		return false;
	}

	return true;
}



// Supply output of "smartctl --version" here.
// returns false on failure. Non-unix newlines in s are ok.
bool SmartctlParser::parse_version(const std::string& s, std::string& version, std::string& version_full)
{
	// e.g. "smartctl version 5.37" or "smartctl 5.39"
	// "smartctl 5.39 2009-06-03 20:10" (cvs versions)
	// "smartctl 5.39 2009-08-08 r2873" (svn versions)
	if (!app_pcre_match("/^smartctl (?:version )?(([0-9][^ \\t\\n\\r]+)(?: [0-9 r:-]+)?)/mi", s, &version_full, &version)) {
		debug_out_error("app", DBG_FUNC_MSG << "No smartctl version information found in supplied string.\n");
		return false;
	}
	hz::string_trim(version_full);

	return true;
}




// check that the version of smartctl output can be parsed with this parser.
bool SmartctlParser::check_version(const std::string& version_str, const std::string& version_full_str)
{
	// tested with 5.1-xx versions (1 - 18), and 5.[20 - 38].
	// note: 5.1-11 (maybe others too) with scsi disk gives non-parsable output (why?).

	// 5.0-24, 5.0-36, 5.0-49 tested with data only, from smartmontool site.
	// can't fully test 5.0-xx, they don't support sata and I have only sata.
	double minimum_version = 5.0;

	double version = 0;
	if (hz::string_is_numeric<double>(version_str, version, false)) {
		if (version >= minimum_version)
			return true;
	}

	return false;
}



// convert e.g. "1,000,204,886,016 bytes" to 1.00 TB [931.51 GiB, 1000204886016 bytes].
// Note: this property is present since 5.33.
std::string SmartctlParser::parse_byte_size(const std::string& str, uint64_t& bytes, bool extended)
{
	// E.g. "500,107,862,016" bytes or "80'060'424'192 bytes" or "80 026 361 856 bytes".
	// French locale inserts 0xA0 as a separator (non-breaking space, _not_ a valid utf8 char).
	// Added '.'-separated too, just in case.
	// Smartctl uses system locale's thousands_sep explicitly.

	// When launching smartctl, we use LANG=C for it, but it works only on POSIX.
	// Also, loading smartctl output files from different locales doesn't really work.

// 	debug_out_dump("app", "Size reported as: " << str << "\n");

	std::vector<std::string> to_replace;
	to_replace.push_back(" ");
	to_replace.push_back("'");
	to_replace.push_back(",");
	to_replace.push_back(".");
	to_replace.push_back(std::string(1, 0xa0));

#ifdef _WIN32
	// if current locale is C, then probably we didn't change it at application
	// startup, so set it now (temporarily). Otherwise, just use the current locale's
	// thousands separator.
	{
		std::string old_locale = hz::locale_c_get();
		hz::ScopedCLocale loc("", old_locale == "C");  // set system locale if the current one is C

		struct lconv* lc = std::localeconv();
		if (lc && lc->thousands_sep && *(lc->thousands_sep)) {
			to_replace.push_back(lc->thousands_sep);
		}
	}  // the locale is restored here
#endif

	to_replace.push_back("bytes");
	std::string s = hz::string_replace_array_copy(hz::string_trim_copy(str), to_replace, "");

	uint64_t v = 0;
	if (hz::string_is_numeric(s, v, false)) {
		bytes = v;
		return hz::format_size(v, true) + (extended ?
				" [" + hz::format_size(v, false) + ", " + hz::number_to_string(v) + " bytes]" : "");
	}

	return std::string();
}



// Parse the section part (with "=== .... ===" header) - info or data sections.
bool SmartctlParser::parse_section(const std::string& header, const std::string& body)
{
	if (app_pcre_match("/START OF INFORMATION SECTION/mi", header)) {
		return parse_section_info(body);

	} else if (app_pcre_match("/START OF READ SMART DATA SECTION/mi", header)) {
		return parse_section_data(body);

	// These sections provide information about actions performed.
	// You may encounter this if e.g. executing "smartctl -a -s on".

	// example contents: "SMART Enabled.".
	} else if (app_pcre_match("/START OF READ SMART DATA SECTION/mi", header)) {
		return true;

	// We don't parse this - it's parsed by the respective command issuer.
	} else if (app_pcre_match("/START OF ENABLE/DISABLE COMMANDS SECTION/mi", header)) {
		return true;

	// This is printed when executing "-t long", etc... . Parsed by respective command issuer.
	} else if (app_pcre_match("/START OF OFFLINE IMMEDIATE AND SELF-TEST SECTION/mi", header)) {
		return true;
	}

	debug_out_warn("app", DBG_FUNC_MSG << "Unknown section encountered.\n");
	debug_out_dump("app", "---------------- Begin unknown section header dump ----------------\n");
	debug_out_dump("app", header << "\n");
	debug_out_dump("app", "----------------- End unknown section header dump -----------------\n");

	return false;  // unknown section
}




// ------------------------------------------------ INFO SECTION


// Parse the info section (without "===" header)
bool SmartctlParser::parse_section_info(const std::string& body)
{
	this->set_data_section_info(body);

	StorageProperty::section_t section = StorageProperty::section_info;

	// split by lines.
	// e.g. Device Model:     ST3500630AS
	pcrecpp::RE re = app_pcre_re("/^([^\\n]+): [ \\t]*(.*)$/miU");  // ungreedy

	std::string name, value;
	pcrecpp::StringPiece input(body);  // position tracker

	while (re.FindAndConsume(&input, &name, &value)) {
		hz::string_trim(name);
		hz::string_trim(value);

		// This is not an ordinary name / value pair, so filter it out (we don't need it anyway).
		// Usually this happens when smart is unsupported or disabled.
		if (app_pcre_match("/mandatory SMART command failed/mi", name))
			continue;

		StorageProperty p;
		p.section = section;
		p.set_name(name);
		p.reported_value = value;

		parse_section_info_property(p);  // set type and the typed value. may change generic_name too.

		add_property(p);
	}

	return true;
}



// Parse a component (one line) of the info section
bool SmartctlParser::parse_section_info_property(StorageProperty& p)
{
	// ---- Info
	if (p.section != StorageProperty::section_info) {
		set_error_msg("Internal parser error.");  // set this so we have something to display
		debug_out_error("app", DBG_FUNC_MSG << "Called with non-info section!\n");
		return false;
	}


	if (app_pcre_match("/Model Family/mi", p.reported_name)
			|| app_pcre_match("/Device Model/mi", p.reported_name)
			|| app_pcre_match("/Serial Number/mi", p.reported_name)
			|| app_pcre_match("/Firmware Version/mi", p.reported_name)
			) {
		p.value_type = StorageProperty::value_type_string;
		p.value_string = p.reported_value;

	} else if (app_pcre_match("/ATA Standard is/mi", p.reported_name)) {
		p.set_name(p.reported_name, "ata_standard", "ATA Standard");
		p.value_type = StorageProperty::value_type_string;
		p.value_string = p.reported_value;

	} else if (app_pcre_match("/Local Time is/mi", p.reported_name)) {
		p.set_name(p.reported_name, "scan_time", "Scanned on");
		p.value_type = StorageProperty::value_type_string;
		p.value_string = p.reported_value;

	} else if (app_pcre_match("/User Capacity/mi", p.reported_name)) {
		p.value_type = StorageProperty::value_type_integer;
		uint64_t v = 0;
		if ((p.readable_value = parse_byte_size(p.reported_value, v, true)).empty()) {
			p.readable_value = "[unknown]";
		} else {
			p.value_integer = v;
		}

	} else if (app_pcre_match("/ATA Version is/mi", p.reported_name)) {
		p.set_name(p.reported_name, "ata_version", "ATA Version");
		p.value_type = StorageProperty::value_type_integer;
		int64_t v = 0;
		if (hz::string_is_numeric(p.reported_value, v, true))  // strict mode
			p.value_integer = v;

	} else if (app_pcre_match("/Device is/mi", p.reported_name)) {
		p.set_name(p.reported_name, "in_smartctl_db", "In Smartctl Database");
		p.value_type = StorageProperty::value_type_bool;
		p.value_bool = (!app_pcre_match("/Not in /mi", p.reported_value));

	} else if (app_pcre_match("/SMART support is/mi", p.reported_name)) {
		// There are two different properties with this name - supported and enabled.
		// Don't put complete messages here - they change across smartctl versions.

		if (app_pcre_match("/Available - device has/mi", p.reported_value)) {
			p.set_name(p.reported_name, "smart_supported", "SMART Supported");
			p.value_type = StorageProperty::value_type_bool;
			p.value_bool = true;

		} else if (app_pcre_match("/Enabled/mi", p.reported_value)) {
			p.set_name(p.reported_name, "smart_enabled", "SMART Enabled");
			p.value_type = StorageProperty::value_type_bool;
			p.value_bool = true;

		} else if (app_pcre_match("/Disabled/mi", p.reported_value)) {
			p.set_name(p.reported_name, "smart_enabled", "SMART Enabled");
			p.value_type = StorageProperty::value_type_bool;
			p.value_bool = false;

		} else if (app_pcre_match("/Unavailable/mi", p.reported_value)) {
			p.set_name(p.reported_name, "smart_supported", "SMART Supported");
			p.value_type = StorageProperty::value_type_bool;
			p.value_bool = false;

		// this should be last - when ambiguous state is detected, usually smartctl
		// retries with other methods and prints one of the above.
		} else if (app_pcre_match("/Ambiguous/mi", p.reported_value)) {
			p.set_name(p.reported_name, "smart_supported", "SMART Supported");
			p.value_type = StorageProperty::value_type_bool;
			p.value_bool = true;  // let's be optimistic - just hope that it doesn't hurt.

		}

	} else {
		debug_out_warn("app", DBG_FUNC_MSG << "Unknown attribute \"" << p.reported_name << "\"\n");
		// this is not an error, just unknown attribute. treat it as string.
		// Don't highlight it with warning, it may just be a new smartctl feature.
		p.value_type = StorageProperty::value_type_string;
		p.value_string = p.reported_value;
	}

	return true;
}




// ------------------------------------------------ DATA SECTION


// Parse the Data section (without "===" header)
bool SmartctlParser::parse_section_data(const std::string& body)
{
	this->set_data_section_data(body);

	// perform any2unix
// 	std::string s = hz::string_any_to_unix_copy(body);

	std::vector<std::string> split_subsections;
	// subsections are separated by double newlines, except the "error log" subsection,
	// which contains double-newline-separated blocks.
	hz::string_split(body, "\n\n", split_subsections, true);

	bool status = false;  // at least one subsection was parsed


	std::vector<std::string> subsections;

	// merge "error log" parts. each part begins with a double-space or "Error nn".
	for (unsigned int i = 0; i < split_subsections.size(); ++i) {
		std::string sub = hz::string_trim_copy(split_subsections[i], "\t\n\r");  // don't trim space
		if (app_pcre_re("^  ").PartialMatch(sub) || app_pcre_re("^Error [0-9]+").PartialMatch(sub)) {
			if (!subsections.empty()) {
				subsections.back() += "\n\n" + sub;  // append to previous part
			} else {
				debug_out_warn("app", DBG_FUNC_MSG << "Error Log's Error block found without any data subsections present.\n");
			}
		} else {  // not an Error block, process as usual
			subsections.push_back(sub);
		}
	}


	// parse each subsection
	for (unsigned int i = 0; i < subsections.size(); ++i) {
		std::string sub = hz::string_trim_copy(subsections[i]);
		if (sub.empty())
			continue;

		if (app_pcre_match("/SMART overall-health self-assessment/mi", sub)) {
			status = parse_section_data_subsection_health(sub) || status;

		} else if (app_pcre_match("/General SMART Values/mi", sub)) {
			status = parse_section_data_subsection_capabilities(sub) || status;

		} else if (app_pcre_match("/SMART Attributes Data Structure/mi", sub)) {
			status = parse_section_data_subsection_attributes(sub) || status;

		} else if (app_pcre_match("/SMART Error Log Version/mi", sub)
				|| app_pcre_match("/Warning: device does not support Error Logging/mi", sub)) {
			status = parse_section_data_subsection_error_log(sub) || status;

		} else if (app_pcre_match("/SMART Self-test log/mi", sub)
				|| app_pcre_match("/Warning: device does not support Self Test Logging/mi", sub)) {
			status = parse_section_data_subsection_selftest_log(sub) || status;

		} else if (app_pcre_match("/SMART Selective self-test log data structure/mi", sub)
				|| app_pcre_match("/Device does not support Selective Self Tests/Logging/mi", sub)) {
			status = parse_section_data_subsection_selective_selftest_log(sub) || status;

		} else {
			debug_out_warn("app", DBG_FUNC_MSG << "Unknown Data subsection encountered.\n");
			debug_out_dump("app", "---------------- Begin unknown section dump ----------------\n");
			debug_out_dump("app", sub << "\n");
			debug_out_dump("app", "----------------- End unknown section dump -----------------\n");
		}
	}

	return status;
}




// -------------------- Health

bool SmartctlParser::parse_section_data_subsection_health(const std::string& sub)
{
	StorageProperty pt;  // template for easy copying
	pt.section = StorageProperty::section_data;
	pt.subsection = StorageProperty::subsection_health;

	std::string name, value;
	if (app_pcre_match("/^([^:\\n]+):[ \\t]*(.*)$/mi", sub, &name, &value)) {
		hz::string_trim(name);
		hz::string_trim(value);

		// only one attribute in this section
		if (app_pcre_match("/SMART overall-health self-assessment/mi", name)) {
			pt.set_name(name, "overall_health", "Overall Health Self-Assessment Test");
			pt.reported_value = value;
			pt.value_type = StorageProperty::value_type_string;
			pt.value_string = pt.reported_value;

			add_property(pt);
		}

		return true;
	}

	return false;
}




// -------------------- Capabilities

bool SmartctlParser::parse_section_data_subsection_capabilities(const std::string& sub_initial)
{
	StorageProperty pt;  // template for easy copying
	pt.section = StorageProperty::section_data;
	pt.subsection = StorageProperty::subsection_capabilities;

	std::string sub = sub_initial;

	// Fix some bugs in smartctl output (pre-5.39-final versions):
	// There is a stale newline in "is in a Vendor Specific state\n.\n" and
	// "is in a Reserved state\n.\n".
// 	app_pcre_replace("/\\n\\.$/mi", ".", &sub);
	app_pcre_replace("/(is in a Vendor Specific state)\\n\\.$/mi", "\\1.", sub);
	app_pcre_replace("/(is in a Reserved state)\\n\\.$/mi", "\\1.", sub);


	// split to lines and merge them into blocks
	std::vector<std::string> lines, blocks;
	hz::string_split(sub, '\n', lines, true);
	bool partial = false;

	for(unsigned int i = 0; i < lines.size(); ++i) {
		std::string line = lines[i];
		if (line.empty() || app_pcre_match("/General SMART Values/mi", line))  // skip the non-informative lines
			continue;
		line += "\n";  // avoid joining lines without separator. this will get stripped anyway.

		if (line.find_first_of(" \t") != 0 && !partial) {  // new blocks don't start with whitespace
			blocks.push_back(std::string());  // new block
			blocks.back() += line;
			if (line.find(":") == std::string::npos)
				partial = true;  // if the name spans several lines (they all start with non-whitespace)
			continue;
		}

		if (partial && line.find(":") != std::string::npos)
			partial = false;

		if (blocks.empty()) {
			debug_out_error("app", DBG_FUNC_MSG << "Non-block related line found!\n");
			blocks.push_back(std::string());  // avoid segfault
		}
		blocks.back() += line;
	}


	// parse each block
	pcrecpp::RE re = app_pcre_re("/([^:]*):\\s*\\(([^)]+)\\)\\s*(.*)/ms");

	bool cap_found = false;  // found at least one capability

	for(unsigned int i = 0; i < blocks.size(); ++i) {
		std::string block = hz::string_trim_copy(blocks[i]);

		std::string name_orig, numvalue_orig, strvalue_orig;

		if (!re.FullMatch(block, &name_orig, &numvalue_orig, &strvalue_orig)) {
			debug_out_error("app", DBG_FUNC_MSG << "Block "
					<< i << " cannot be parsed.\n");
			debug_out_dump("app", "---------------- Begin unparsable block dump ----------------\n");
			debug_out_dump("app", block << "\n");
			debug_out_dump("app", "----------------- End unparsable block dump -----------------\n");
			continue;
		}

		// flatten:
		std::string name = hz::string_trim_copy(hz::string_remove_adjacent_duplicates_copy(
				hz::string_replace_chars_copy(name_orig, "\t\n", ' '), ' '));

		std::string strvalue = hz::string_trim_copy(hz::string_remove_adjacent_duplicates_copy(
				hz::string_replace_chars_copy(strvalue_orig, "\t\n", ' '), ' '));

		int numvalue = -1;
		if (!hz::string_is_numeric<int>(hz::string_trim_copy(numvalue_orig), numvalue, false)) {  // this will autodetect number base.
			debug_out_warn("app", DBG_FUNC_MSG
					<< "Numeric value: \"" << numvalue_orig << "\" cannot be parsed as number.\n");
		}

		// 		debug_out_dump("app", "name: \"" << name << "\"\n\tnumvalue: \"" << numvalue
		// 				<< "\"\n\tstrvalue: \"" << strvalue << "\"\n\n");


		// Time length properties
		if (hz::string_erase_right_copy(strvalue, ".") == "minutes"
				|| hz::string_erase_right_copy(strvalue, ".") == "seconds") {

			// const int numvalue_unmod = numvalue;

			if (hz::string_erase_right_copy(strvalue, ".") == "minutes")
				numvalue *= 60;  // convert to seconds

			// add as a time property
			StorageProperty p(pt);
			p.set_name(name);
			p.reported_value = numvalue_orig + " | " + strvalue_orig;  // well, not really as reported, but still...
			p.value_type = StorageProperty::value_type_time_length;
			p.value_time_length = numvalue;  // always in seconds

			// Set some generic names on the recognized ones
			parse_section_data_internal_capabilities(p);

			add_property(p);
			cap_found = true;


		// StorageCapability properties (capabilities are flag lists)
		} else {

			StorageProperty p(pt);
			p.set_name(name);
			p.reported_value = numvalue_orig + " | " + strvalue_orig;  // well, not really as reported, but still...
			p.value_type = StorageProperty::value_type_capability;

			p.value_capability.reported_flag_value = numvalue_orig;
			p.value_capability.flag_value = static_cast<int16_t>(numvalue);  // full flag value
			p.value_capability.reported_strvalue = strvalue_orig;

			// split capability lines into a vector. every flag sentence ends with "."
			hz::string_split(strvalue, '.', p.value_capability.strvalues, true);
			for (StorageCapability::strvalue_list_t::iterator iter = p.value_capability.strvalues.begin();
					iter != p.value_capability.strvalues.end(); ++iter) {
				hz::string_trim(*iter);
			}

			// find some special capabilities we're interested in and add them. p is unmodified.
			parse_section_data_internal_capabilities(p);

			add_property(p);
			cap_found = true;
		}

	}

	if (!cap_found)
		set_error_msg("No capabilities found in Capabilities section.");

	return cap_found;
}





// Check the capabilities for internal properties we can use.
bool SmartctlParser::parse_section_data_internal_capabilities(StorageProperty& cap)
{
	// Some special capabilities we're interested in.

	// Note: Smartctl gradually changed spelling Off-line to Offline in some messages.
	// Also, some capitalization was changed (so the regexps are caseless).

	// "Offline data collection not supported." (at all) - we don't need to check this,
	// because we look for immediate/automatic anyway.

	// "was never started", "was completed without error", "is in progress",
	// "was suspended by an interrupting command from host", etc...
	pcrecpp::RE re_offline_status = app_pcre_re("/^(Off-?line data collection) activity (?:is|was) (.*)$/mi");
	// "Enabled", "Disabled". May not show up on older smartctl (< 5.1.10), so no way of knowing there.
	pcrecpp::RE re_offline_enabled = app_pcre_re("/^(Auto Off-?line Data Collection):[ \\t]*(.*)$/mi");
	pcrecpp::RE re_offline_immediate = app_pcre_re("/^(SMART execute Off-?line immediate)$/mi");
	// "No Auto Offline data collection support.", "Auto Offline data collection on/off support.".
	pcrecpp::RE re_offline_auto = app_pcre_re("/^(No |)(Auto Off-?line data collection (?:on\\/off )?support)$/mi");
	// Same as above (smartctl <= 5.1-18). "No Automatic timer ON/OFF support."
	pcrecpp::RE re_offline_auto2 = app_pcre_re("/^(No |)(Automatic timer ON\\/OFF support)$/mi");
	pcrecpp::RE re_offline_suspend = app_pcre_re("/^(?:Suspend|Abort) (Off-?line collection upon new command)$/mi");
	pcrecpp::RE re_offline_surface = app_pcre_re("/^(No |)(Off-?line surface scan supported)$/mi");
	pcrecpp::RE re_offline_time = app_pcre_re("/^(Total time to complete Off-?line data collection)/mi");  // match on name!

	pcrecpp::RE re_selftest_status = app_pcre_re("/^Self-test execution status/mi");  // match on name
	pcrecpp::RE re_selftest_support = app_pcre_re("/^(No |)(Self-test supported)$/mi");
	pcrecpp::RE re_conv_selftest_support = app_pcre_re("/^(No |)(Conveyance Self-test supported)$/mi");
	pcrecpp::RE re_selective_selftest_support = app_pcre_re("/^(No |)(Selective Self-test supported)$/mi");
	pcrecpp::RE re_selftest_short_time = app_pcre_re("/^(Short self-test routine recommended polling time)/mi");  // match on name!
	pcrecpp::RE re_selftest_long_time = app_pcre_re("/^(Extended self-test routine recommended polling time)/mi");  // match on name!
	pcrecpp::RE re_conv_selftest_time = app_pcre_re("/^(Conveyance self-test routine recommended polling time)/mi");  // match on name!

	pcrecpp::RE re_sct_status = app_pcre_re("/^(SCT Status supported)$/mi");
	pcrecpp::RE re_sct_control = app_pcre_re("/^(SCT Feature Control supported)$/mi");  // means can change logging interval
	pcrecpp::RE re_sct_data = app_pcre_re("/^(SCT Data Table supported)$/mi");

	if (cap.section != StorageProperty::section_data || cap.subsection != StorageProperty::subsection_capabilities) {
		debug_out_error("app", DBG_FUNC_MSG << "Non-capability property passed.\n");
		return false;
	}




	// match on name:
	if (re_selftest_status.PartialMatch(cap.reported_name)) {
		// The last self-test status. break up into pieces.

		StorageProperty p;
		p.section = StorageProperty::section_internal;
		p.set_name("last_selftest_status");
		p.value_type = StorageProperty::value_type_selftest_entry;
		p.value_selftest_entry.test_num = 0;
		p.value_selftest_entry.remaining_percent = -1;  // unknown or n/a

		// check for lines in capability vector
		for (StorageCapability::strvalue_list_t::const_iterator iter = cap.value_capability.strvalues.begin();
				iter != cap.value_capability.strvalues.end(); ++iter) {
			std::string value;

			if (app_pcre_match("/^([0-9]+)% of test remaining/mi", *iter, &value)) {
				uint8_t v = 0;
				if (hz::string_is_numeric(value, v))
					p.value_selftest_entry.remaining_percent = v;

			} else if (app_pcre_match("/^(The previous self-test routine completed without error or no .*)/mi", *iter, &value)) {
				p.value_selftest_entry.status_str = value;
				p.value_selftest_entry.status = StorageSelftestEntry::status_completed_no_error;

			} else if (app_pcre_match("/^(The self-test routine was aborted by the host)/mi", *iter, &value)) {
				p.value_selftest_entry.status_str = value;
				p.value_selftest_entry.status = StorageSelftestEntry::status_aborted_by_host;

			} else if (app_pcre_match("/^(The self-test routine was interrupted by the host with a hard.*)/mi", *iter, &value)) {
				p.value_selftest_entry.status_str = value;
				p.value_selftest_entry.status = StorageSelftestEntry::status_interrupted;

			} else if (app_pcre_match("/^(A fatal error or unknown test error occurred while the device was executing its .*)/mi", *iter, &value)) {
				p.value_selftest_entry.status_str = value;
				p.value_selftest_entry.status = StorageSelftestEntry::status_fatal_or_unknown;

			} else if (app_pcre_match("/^(The previous self-test completed having a test element that failed and the test element that failed is not known)/mi", *iter, &value)) {
				p.value_selftest_entry.status_str = value;
				p.value_selftest_entry.status = StorageSelftestEntry::status_compl_unknown_failure;

			} else if (app_pcre_match("/^(The previous self-test completed having the electrical element of the test failed)/mi", *iter, &value)) {
				p.value_selftest_entry.status_str = value;
				p.value_selftest_entry.status = StorageSelftestEntry::status_compl_electrical_failure;

			} else if (app_pcre_match("/^(The previous self-test completed having the servo .*)/mi", *iter, &value)) {
				p.value_selftest_entry.status_str = value;
				p.value_selftest_entry.status = StorageSelftestEntry::status_compl_servo_failure;

			} else if (app_pcre_match("/^(The previous self-test completed having the read element of the test failed)/mi", *iter, &value)) {
				p.value_selftest_entry.status_str = value;
				p.value_selftest_entry.status = StorageSelftestEntry::status_compl_read_failure;

			} else if (app_pcre_match("/^(The previous self-test completed having a test element that failed and the device is suspected of having handling damage)/mi", *iter, &value)) {
				p.value_selftest_entry.status_str = value;
				p.value_selftest_entry.status = StorageSelftestEntry::status_compl_handling_damage;

			// samsung bug (?), as per smartctl sources.
			} else if (app_pcre_match("/^(The previous self-test routine completed with unknown result or self-test .*)/mi", *iter, &value)) {
				p.value_selftest_entry.status_str = value;
				p.value_selftest_entry.status = StorageSelftestEntry::status_compl_unknown_failure;  // we'll use this again (correct?)

			} else if (app_pcre_match("/^(Self-test routine in progress)/mi", *iter, &value)) {
				p.value_selftest_entry.status_str = value;
				p.value_selftest_entry.status = StorageSelftestEntry::status_in_progress;

			} else if (app_pcre_match("/^(Reserved)/mi", *iter, &value)) {
				p.value_selftest_entry.status_str = value;
				p.value_selftest_entry.status = StorageSelftestEntry::status_reserved;
			}
		}

		add_property(p);

		return true;
	}


	// Check the time-related ones first.
	// Note: We only modify the existing property here!
	// Section is unmodified.
	if (cap.value_type == StorageProperty::value_type_time_length) {

		if (re_offline_time.PartialMatch(cap.reported_name)) {
			cap.generic_name = "iodc_total_time_length";

		} else if (re_selftest_short_time.PartialMatch(cap.reported_name)) {
			cap.generic_name = "short_total_time_length";

		} else if (re_selftest_long_time.PartialMatch(cap.reported_name)) {
			cap.generic_name = "long_total_time_length";

		} else if (re_conv_selftest_time.PartialMatch(cap.reported_name)) {
			cap.generic_name = "conveyance_total_time_length";
		}

		return true;
	}

	// Extract subcapabilities from capability vectors and assign to "internal" section.
	if (cap.value_type == StorageProperty::value_type_capability) {

		// check for lines in capability vector
		for (StorageCapability::strvalue_list_t::const_iterator iter = cap.value_capability.strvalues.begin();
				iter != cap.value_capability.strvalues.end(); ++iter) {

			// debug_out_dump("app", "Looking for internal capability in: \"" << (*iter) << "\"\n");

			StorageProperty p;
			p.section = StorageProperty::section_internal;
			// Note: We don't set reported_value on internal properties.

			std::string name, value;

			if (re_offline_status.PartialMatch(*iter, &name, &value)) {
				p.set_name(name, "odc_status");
				p.value_type = StorageProperty::value_type_string;
				p.value_string = hz::string_trim_copy(value);

			} else if (re_offline_enabled.PartialMatch(*iter, &name, &value)) {
				p.set_name(name, "aodc_enabled");
				p.value_type = StorageProperty::value_type_bool;
				p.value_bool = (hz::string_trim_copy(value) == "Enabled");

			} else if (re_offline_immediate.PartialMatch(*iter, &name)) {
				p.set_name(name, "iodc_support");
				p.value_type = StorageProperty::value_type_bool;
				p.value_bool = true;

			} else if (re_offline_auto.PartialMatch(*iter, &value, &name) || re_offline_auto2.PartialMatch(*iter, &value, &name)) {
				p.set_name(name, "aodc_support", "Automatic Offline Data Collection toggle support");
				p.value_type = StorageProperty::value_type_bool;
				p.value_bool = (hz::string_trim_copy(value) != "No");

			} else if (re_offline_suspend.PartialMatch(*iter, &value, &name)) {
				p.set_name(name, "iodc_command_suspends", "Offline Data Collection suspends upon new command");
				p.value_type = StorageProperty::value_type_bool;
				p.value_bool = (hz::string_trim_copy(value) == "Suspend");

			} else if (re_offline_surface.PartialMatch(*iter, &value, &name)) {
				p.set_name(name, "odc_surface_scan_support");
				p.value_type = StorageProperty::value_type_bool;
				p.value_bool = (hz::string_trim_copy(value) != "No");


			} else if (re_selftest_support.PartialMatch(*iter, &value, &name)) {
				p.set_name(name, "selftest_support");
				p.value_type = StorageProperty::value_type_bool;
				p.value_bool = (hz::string_trim_copy(value) != "No");

			} else if (re_conv_selftest_support.PartialMatch(*iter, &value, &name)) {
				p.set_name(name, "conveyance_support");
				p.value_type = StorageProperty::value_type_bool;
				p.value_bool = (hz::string_trim_copy(value) != "No");

			} else if (re_selective_selftest_support.PartialMatch(*iter, &value, &name)) {
				p.set_name(name, "selective_selftest_support");
				p.value_type = StorageProperty::value_type_bool;
				p.value_bool = (hz::string_trim_copy(value) != "No");


			} else if (re_sct_status.PartialMatch(*iter, &name)) {
				p.set_name(name, "sct_status_support");
				p.value_type = StorageProperty::value_type_bool;
				p.value_bool = true;

			} else if (re_sct_control.PartialMatch(*iter, &name)) {
				p.set_name(name, "sct_control_support");
				p.value_type = StorageProperty::value_type_bool;
				p.value_bool = true;

			} else if (re_sct_data.PartialMatch(*iter, &name)) {
				p.set_name(name, "sct_data_support");
				p.value_type = StorageProperty::value_type_bool;
				p.value_bool = true;
			}

			if (!p.empty())
				add_property(p);
		}

		return true;
	}

	debug_out_error("app", DBG_FUNC_MSG << "Capability property has invalid type \""
			<< StorageProperty::get_value_type_name(cap.value_type) << "\".\n");

	return false;
}





// -------------------- Attributes

bool SmartctlParser::parse_section_data_subsection_attributes(const std::string& sub)
{
	StorageProperty pt;  // template for easy copying
	pt.section = StorageProperty::section_data;
	pt.subsection = StorageProperty::subsection_attributes;

	// split to lines
	std::vector<std::string> lines;
	hz::string_split(sub, '\n', lines, true);

	// Format notes:
	// * Before 5.1-14, no UPDATED column was present.
	// * Most, but not all attribute names are with underscores. However, I encountered one
	// named "Head flying hours". So, parse until we encounter the next column.
	// * One WD drive had non-integer flags, something like "PO--C-", with several
	// lines of their descriptions after the attributes block (each line started with spaces and |).
	// * SSD drives may show "---" in value/worst/threshold fields.

	// "  1 Raw_Read_Error_Rate     0x000f   115   099   006    Pre-fail  Always       -       90981479"
	// " 12 Power_Cycle_Count       0x0000   ---   ---   ---    Old_age   Offline      -       167"

	bool attr_found = false;  // at least one attribute was found
	bool attr_format_with_updated = false;  // UPDATED column present

	std::string base_re = "[ \\t]*([0-9]+) ([^\\t\\n]+)[ \\t]+((?:0x[a-fA-F0-9]+)|(?:[A-Z-]{2,}))[ \\t]+"  // name / flag
			"([0-9-]+)[ \\t]+([0-9-]+)[ \\t]+([0-9-]+)[ \\t]+"  // value / worst / threshold
			"([^ \\t\\n]+)[ \\t]+";  // type

	pcrecpp::RE re_up = app_pcre_re("/" + base_re
			+ "([^ \\t\\n]+)[ \\t]+([^ \\t\\n]+)[ \\t]+(.+)[ \\t]*/mi");  // updated / when_failed / raw

	pcrecpp::RE re_noup = app_pcre_re("/" + base_re
			+ "([^ \\t\\n]+)[ \\t]+(.+)[ \\t]*/mi");  // when_failed / raw

	pcrecpp::RE re_flag_descr = app_pcre_re("/^[\\t ]+\\|/mi");


	for(unsigned int i = 0; i < lines.size(); ++i) {
		std::string line = lines[i];

		// skip the non-informative lines
		if (line.empty() || app_pcre_match("/SMART Attributes with Thresholds/mi", line))
			continue;

		if (app_pcre_match("/ATTRIBUTE_NAME/mi", line)) {
			attr_format_with_updated = (app_pcre_match("/UPDATED/mi", line));
			continue;  // we don't need this line
		}

		if (re_flag_descr.PartialMatch(line)) {
			continue;  // skip flag description lines
		}

		if (app_pcre_match("/Data Structure revision number/mi", line)) {
			pcrecpp::RE re = app_pcre_re("/^([^:\\n]+):[ \\t]*(.*)$/mi");
			std::string name, value;
			if (re.PartialMatch(line, &name, &value)) {
				hz::string_trim(name);
				hz::string_trim(value);
				int64_t value_num = 0;
				hz::string_is_numeric(value, value_num, false);

				StorageProperty p(pt);
				p.set_name(name);
				p.reported_value = value;
				p.value_type = StorageProperty::value_type_integer;
				p.value_integer = value_num;

				add_property(p);
				attr_found = true;
			}


		} else {  // A line in attribute table

			std::string id, name, flag, value, worst, threshold, attr_type,
					update_type, when_failed, raw_value;

			bool matched = true;

			if (attr_format_with_updated) {
				if (!re_up.FullMatch(line, &id, &name, &flag, &value, &worst, &threshold, &attr_type,
						&update_type, &when_failed, &raw_value)) {
					matched = false;
					debug_out_warn("app", DBG_FUNC_MSG << "Cannot parse attribute line.\n");
				}

			} else {  // no UPDATED column
				if (!re_noup.FullMatch(line, &id, &name, &flag, &value, &worst, &threshold, &attr_type,
						&when_failed, &raw_value)) {
					matched = false;
					debug_out_warn("app", DBG_FUNC_MSG << "Cannot parse attribute line.\n");
				}
			}

			if (!matched) {
				debug_out_dump("app", "------------ Begin unparsable attribute line dump ------------\n");
				debug_out_dump("app", line << "\n");
				debug_out_dump("app", "------------- End unparsable attribute line dump -------------\n");
				continue;  // continue to the next line
			}


			StorageAttribute a;
			hz::string_is_numeric(hz::string_trim_copy(id), a.id, true, 10);
			a.flag = hz::string_trim_copy(flag);
			uint8_t norm_value = 0, worst_value = 0, threshold_value = 0;

			if (hz::string_is_numeric(hz::string_trim_copy(value), norm_value, true, 10)) {
				a.value = norm_value;
			}
			if (hz::string_is_numeric(hz::string_trim_copy(worst), worst_value, true, 10)) {
				a.worst = worst_value;
			}
			if (hz::string_is_numeric(hz::string_trim_copy(threshold), threshold_value, true, 10)) {
				a.threshold = threshold_value;
			}

			a.attr_type = (attr_type == "Pre-fail" ? StorageAttribute::attr_type_prefail
					: (attr_type == "Old_age" ? StorageAttribute::attr_type_oldage : StorageAttribute::attr_type_unknown));

			a.update_type = (update_type == "Always" ? StorageAttribute::update_type_always
					: (update_type == "Offline" ? StorageAttribute::update_type_offline : StorageAttribute::update_type_unknown));

			a.when_failed = StorageAttribute::fail_time_unknown;
			hz::string_trim(when_failed);
			if (when_failed == "-") {
				a.when_failed = StorageAttribute::fail_time_none;
			} else if (when_failed == "In_the_past") {
				a.when_failed = StorageAttribute::fail_time_past;
			} else if (when_failed == "FAILING_NOW") {
				a.when_failed = StorageAttribute::fail_time_now;
			}

			a.raw_value = hz::string_trim_copy(raw_value);
			hz::string_is_numeric(hz::string_trim_copy(raw_value), a.raw_value_int, false);  // same as raw_value, but parsed as int.

			StorageProperty p(pt);
			p.set_name(hz::string_trim_copy(name));
			p.reported_value = line;  // use the whole line here
			p.value_type = StorageProperty::value_type_attribute;
			p.value_attribute = a;

			add_property(p);
			attr_found = true;
		}

	}

	if (!attr_found)
		set_error_msg("No attributes found in Attributes section.");

	return attr_found;
}




// -------------------- Error Log

bool SmartctlParser::parse_section_data_subsection_error_log(const std::string& sub)
{
	StorageProperty pt;  // template for easy copying
	pt.section = StorageProperty::section_data;
	pt.subsection = StorageProperty::subsection_error_log;

	// Note: The format of this section was changed somewhere between 5.0-x and 5.30.
	// The old format is doesn't really give any useful info, and whatever's left is somewhat
	// parsable by this parser. Can't really improve that.
	// Also, type (e.g. UNC) is not always present (depends on the drive I guess).

	bool data_found = false;

	// Error log version
	{
		pcrecpp::RE re = app_pcre_re("/^(SMART Error Log Version):[ \\t]*(.*)$/mi");

		std::string name, value;
		if (re.PartialMatch(sub, &name, &value)) {
			hz::string_trim(name);
			hz::string_trim(value);

			StorageProperty p(pt);
			p.set_name(name, "error_log_version");
			p.reported_value = value;
			p.value_type = StorageProperty::value_type_integer;

			int64_t value_num = 0;
			hz::string_is_numeric(value, value_num, false);
			p.value_integer = value_num;

			add_property(p);
			data_found = true;
		}
	}

	// Error log support
	{
		pcrecpp::RE re = app_pcre_re("/^(Warning: device does not support Error Logging)$/mi");

		if (re.PartialMatch(sub)) {
			StorageProperty p(pt);
			p.set_name("error_log_unsupported");
			p.readable_name = "Warning";
			p.readable_value = "Device does not support error logging";
			add_property(p);
		}
	}

	// Error log enty count
	{
		// note: these represent the same information
		pcrecpp::RE re1 = app_pcre_re("/^ATA Error Count:[ \\t]*([0-9]+)/mi");
		pcrecpp::RE re2 = app_pcre_re("/^No Errors Logged$/mi");

		std::string value;
		if (re1.PartialMatch(sub, &value) || re2.PartialMatch(sub)) {
			hz::string_trim(value);

			StorageProperty p(pt);
			p.set_name("ATA Error Count", "error_count");
			p.reported_value = value;
			p.value_type = StorageProperty::value_type_integer;

			int64_t value_num = 0;
			if (!re2.PartialMatch(sub)) {  // if no errors, when value should be zero. otherwise, this:
				hz::string_is_numeric(value, value_num, false);
			}
			p.value_integer = value_num;

			add_property(p);
			data_found = true;
		}
	}

	// individual errors
	{
		// split by blocks
		pcrecpp::RE re_block = app_pcre_re("/^((Error[ \\t]*([0-9]+))[ \\t]*occurred at disk power-on lifetime:[ \\t]*([0-9]+) hours.*(?:\\n(?:  |\\n  ).*)*)/mi");

		// "  When the command that caused the error occurred, the device was active or idle."
		// Note: For "in an unknown state" - remove first two words.
		pcrecpp::RE re_state = app_pcre_re("/occurred, the device was[ \\t]*(?: in)?(?: an?)?[ \\t]+([^.\\n]*)\\.?/mi");
		// "  84 51 2c 71 cd 3f e6  Error: ICRC, ABRT 44 sectors at LBA = 0x063fcd71 = 104844657"
		// "  40 51 00 f5 41 61 e0  Error: UNC at LBA = 0x006141f5 = 6373877"
		pcrecpp::RE re_type = app_pcre_re("/[ \\t]+Error:[ \\t]*([ ,a-z]+)[ \\t]+((?:[0-9]+|at )[ \\t]*.*)$/mi");

		std::string block, name, value_num, value_time;
		pcrecpp::StringPiece input(sub);  // position tracker

		while (re_block.FindAndConsume(&input, &block, &name, &value_num, &value_time)) {
			hz::string_trim(block);
			hz::string_trim(value_num);
			hz::string_trim(value_time);
			// debug_out_dump("app", "\nBLOCK -------------------------------\n" << block);

			std::string state, etypes_str, emore;
			re_state.PartialMatch(block, &state);
			re_type.PartialMatch(block, &etypes_str, &emore);

			StorageProperty p(pt);
			p.set_name(hz::string_trim_copy(name));  // "Error 6"
			p.reported_value = block;
			p.value_type = StorageProperty::value_type_error_block;

			hz::string_is_numeric(value_num, p.value_error_block.error_num, false);
			hz::string_is_numeric(value_time, p.value_error_block.lifetime_hours, false);

			std::vector<std::string> etypes;
			hz::string_split(etypes_str, ",", etypes, true);
			for (std::vector<std::string>::iterator iter = etypes.begin(); iter != etypes.end(); ++iter) {
				hz::string_trim(*iter);
			}

			p.value_error_block.device_state = hz::string_trim_copy(state);
			p.value_error_block.reported_types = etypes;
			p.value_error_block.type_more_info = hz::string_trim_copy(emore);

			add_property(p);
			data_found = true;
		}


	}

	// the whole subsection
	{
		StorageProperty p(pt);
		p.set_name("SMART Error Log", "error_log");
		p.reported_value = sub;
		p.value_type = StorageProperty::value_type_string;
		p.value_string = p.reported_value;

		add_property(p);
		data_found = true;
	}

	// We may further split this subsection by Error blocks, but it's unnecessary -
	// the data is too advanced to be of any use if parsed.

	return data_found;
}




// -------------------- Selftest Log

bool SmartctlParser::parse_section_data_subsection_selftest_log(const std::string& sub)
{
	StorageProperty pt;  // template for easy copying
	pt.section = StorageProperty::section_data;
	pt.subsection = StorageProperty::subsection_selftest_log;

	// Self-test log contains:
	// * structure revision number
	// * a list of current / previous tests performed, with each having:
	// num (the higher - the older).
	// test_description (Extended offline / Short offline / Conveyance offline / ... ?)
	// status (completed without error, interrupted (reason), aborted, fatal or unknown error, ?)
	// remaining % (this will be 00% for completed, and may be > 0 for interrupted).
	// lifetime (hours) - int.
	// LBA_of_first_error - "-" or int ?

	bool data_found = false;  // true if something was found.

	// The whole subsection
	{
		StorageProperty p(pt);
		p.set_name("SMART Self-test log", "selftest_log");
		p.reported_value = sub;
		p.value_type = StorageProperty::value_type_string;
		p.value_string = p.reported_value;

		add_property(p);
		data_found = true;
	}


	// Self-test log support
	{
		pcrecpp::RE re = app_pcre_re("/^(Warning: device does not support Self Test Logging)$/mi");

		if (re.PartialMatch(sub)) {
			StorageProperty p(pt);
			p.set_name("selftest_log_unsupported");
			p.readable_name = "Warning";
			p.readable_value = "Device does not support self-test logging";
			add_property(p);
		}
	}

	// Self-test log version
	{
		// newer smartctl (since smartctl 5.1-16)
		pcrecpp::RE re1 = app_pcre_re("/(SMART Self-test log structure[^\\n0-9]*)([^ \\n]+)[ \\t]*$/mi");
		// older smartctl
		pcrecpp::RE re2 = app_pcre_re("/(SMART Self-test log, version number[^\\n0-9]*)([^ \\n]+)[ \\t]*$/mi");

		std::string name, value;
		if (re1.PartialMatch(sub, &name, &value) || re2.PartialMatch(sub, &name, &value)) {
			hz::string_trim(value);

			StorageProperty p(pt);
			p.set_name(hz::string_trim_copy(name), "selftest_log_version");
			p.reported_value = value;
			p.value_type = StorageProperty::value_type_integer;

			int64_t value_num = 0;
			hz::string_is_numeric(value, value_num, false);
			p.value_integer = value_num;

			add_property(p);
			data_found = true;
		}
	}


	int64_t test_count = 0;  // type is of p.value_integer


	// TODO: Support SCSI (it has different output format).

	// individual entries
	{
		// split by columns.
		// num, type, status, remaining, hours, lba (optional).
		pcrecpp::RE re = app_pcre_re("/^(#[ \\t]*([0-9]+)[ \\t]+(\\S+(?: \\S+)*)  [ \\t]*(\\S.*) [ \\t]*([0-9]+%)  [ \\t]*([0-9]+)[ \\t]*((?:  [ \\t]*\\S.*)?))$/mi");

		std::string line, num, type, status_str, remaining, hours, lba;
		pcrecpp::StringPiece input(sub);  // position tracker

		while (re.FindAndConsume(&input, &line, &num, &type, &status_str, &remaining, &hours, &lba)) {
			hz::string_trim(num);

			StorageProperty p(pt);
			p.set_name("Self-test entry " + num);
			p.reported_value = hz::string_trim_copy(line);
			p.value_type = StorageProperty::value_type_selftest_entry;

			hz::string_is_numeric(num, p.value_selftest_entry.test_num, false);
			hz::string_is_numeric(hz::string_trim_copy(remaining), p.value_selftest_entry.remaining_percent, false);
			hz::string_is_numeric(hz::string_trim_copy(hours), p.value_selftest_entry.lifetime_hours, false);

			p.value_selftest_entry.type = hz::string_trim_copy(type);
			p.value_selftest_entry.lba_of_first_error = hz::string_trim_copy(lba);
			// old smartctls didn't print anything for lba if none, but newer ones print "-". normalize.
			if (p.value_selftest_entry.lba_of_first_error == "")
				p.value_selftest_entry.lba_of_first_error = "-";

			hz::string_trim(status_str);
			StorageSelftestEntry::status_t status = StorageSelftestEntry::status_unknown;

			// don't match end - some of them are not complete here
			if (app_pcre_match("/^Completed without error/mi", status_str)) {
				status = StorageSelftestEntry::status_completed_no_error;
			} else if (app_pcre_match("/^Aborted by host/mi", status_str)) {
				status = StorageSelftestEntry::status_aborted_by_host;
			} else if (app_pcre_match("/^Interrupted \\(host reset\\)/mi", status_str)) {
				status = StorageSelftestEntry::status_interrupted;
			} else if (app_pcre_match("/^Fatal or unknown error/mi", status_str)) {
				status = StorageSelftestEntry::status_fatal_or_unknown;
			} else if (app_pcre_match("/^Completed: unknown failure/mi", status_str)) {
				status = StorageSelftestEntry::status_compl_unknown_failure;
			} else if (app_pcre_match("/^Completed: electrical failure/mi", status_str)) {
				status = StorageSelftestEntry::status_compl_electrical_failure;
			} else if (app_pcre_match("/^Completed: servo\\/seek failure/mi", status_str)) {
				status = StorageSelftestEntry::status_compl_servo_failure;
			} else if (app_pcre_match("/^Completed: read failure/mi", status_str)) {
				status = StorageSelftestEntry::status_compl_read_failure;
			} else if (app_pcre_match("/^Completed: handling damage/mi", status_str)) {
				status = StorageSelftestEntry::status_compl_handling_damage;
			} else if (app_pcre_match("/^Self-test routine in progress/mi", status_str)) {
				status = StorageSelftestEntry::status_in_progress;
			} else if (app_pcre_match("/^Unknown\\/reserved test status/mi", status_str)) {
				status = StorageSelftestEntry::status_reserved;
			}

			p.value_selftest_entry.status_str = status_str;
			p.value_selftest_entry.status = status;

			add_property(p);
			data_found = true;

			++test_count;
		}
	}


	// number of tests.
	// Note: "No self-tests have been logged" is sometimes absent, so don't rely on it.
	{
		StorageProperty p(pt);
		p.set_name("Number of entries in self-test log", "selftest_num_entries");
		// p.reported_value;  // nothing
		p.value_type = StorageProperty::value_type_integer;
		p.value_integer = test_count;

		add_property(p);
		data_found = true;
	}


	return data_found;
}




// -------------------- Selective Selftest Log

bool SmartctlParser::parse_section_data_subsection_selective_selftest_log(const std::string& sub)
{
	StorageProperty pt;  // template for easy copying
	pt.section = StorageProperty::section_data;
	pt.subsection = StorageProperty::subsection_selective_selftest_log;

	// Selective self-test log contains:
	// * 5 (maybe less/more?) test entries, each with
	// 		span / min_lba / max_lba / current_test_status.
	// * flags
	// * notes

	bool data_found = false;  // true if something was found.

	// the whole subsection
	{
		StorageProperty p(pt);
		p.set_name("SMART Selective self-test log", "selective_selftest_log");
		p.reported_value = sub;
		p.value_type = StorageProperty::value_type_string;
		p.value_string = p.reported_value;

		add_property(p);
		data_found = true;
	}


	// supported / unsupported
	{
		StorageProperty p(pt);
		p.set_name("Selective self-tests supported", "selective_selftest_supported");

		// p.reported_value;  // nothing
		p.value_type = StorageProperty::value_type_bool;
		p.value_bool = !app_pcre_match("/Device does not support Selective Self Tests\\/Logging/mi", sub);

		add_property(p);
		data_found = true;
	}

	return data_found;
}





// adds a property into property list, looks up and sets its description.
// Yes, there's no place for this in the Parser, but whatever...
void SmartctlParser::add_property(StorageProperty p)
{
	storage_property_autoset_description(p);
	storage_property_autoset_warning(p);
	storage_property_autoset_warning_descr(p);  // append warning to description

	properties_.push_back(p);
}







