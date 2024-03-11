/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2022 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef SMARTCTL_PARSER_H
#define SMARTCTL_PARSER_H

#include <string>
#include <vector>
#include <memory>
#include <expected>

#include "ata_storage_property.h"
#include "smartctl_parser_types.h"
#include "hz/error_container.h"



enum class SmartctlParserError {
	EmptyInput,
	UnsupportedFormat,
	SyntaxError,
	NoVersion,
	IncompatibleVersion,
	NoSections,
	UnknownSection,  ///< Local parsing function error
	InternalError,
	NoSubsectionsParsed,
	DataError,
};



/// Smartctl (S)ATA text output parser.
/// Note: ALL parse_* functions (except parse_full() and parse_version())
/// expect data in unix-newline format!
class SmartctlParser {
	protected:

		// Defaulted but hidden
		SmartctlParser() = default;


	public:

		// Deleted
		SmartctlParser(const SmartctlParser& other) = default;

		// Deleted
		SmartctlParser(SmartctlParser&& other) = delete;

		// Deleted
		SmartctlParser& operator=(const SmartctlParser& other) = delete;

		// Deleted
		SmartctlParser& operator=(SmartctlParser&& other) = delete;

		/// Virtual member requirement
		virtual ~SmartctlParser() = default;


		/// Create an instance of this class.
		/// \return nullptr if no such class exists
		static std::unique_ptr<SmartctlParser> create(SmartctlParserType type);


		/// Parse full "smartctl -x" output.
		/// Note: Once parsed, this function cannot be called again.
		virtual hz::ExpectedVoid<SmartctlParserError> parse_full(const std::string& full) = 0;


		/// Detect smartctl output type (text, json).
		[[nodiscard]] static hz::ExpectedValue<SmartctlParserType, SmartctlParserError> detect_output_type(const std::string& output);


		/// Get "full" data, as passed to parse_full().
		[[nodiscard]] std::string get_data_full() const;

		/// Get parse result properties
		[[nodiscard]] const std::vector<AtaStorageProperty>& get_properties() const;


	protected:

		/// Add a property into property list, look up and set its description
		void add_property(AtaStorageProperty p);

		/// Set "full" data ("smartctl -x" output), json or text.
		void set_data_full(const std::string& s);


	private:

		std::vector<AtaStorageProperty> properties_;  ///< Parsed data properties
		std::string data_full_;  ///< full data, filled by parse_full()

};




#endif

/// @}
