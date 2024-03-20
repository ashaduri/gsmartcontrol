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

#include <string_view>
#include <vector>
#include <memory>

#include "ata_storage_property.h"
#include "smartctl_parser_types.h"
#include "hz/error_container.h"
#include "storage_property_repository.h"


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
	KeyNotFound,
};



/// Smartctl output parser.
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
		static std::unique_ptr<SmartctlParser> create(SmartctlParserType type, SmartctlOutputFormat format);


		/// Parse full "smartctl -x" output.
		/// Note: Once parsed, this function cannot be called again.
		virtual hz::ExpectedVoid<SmartctlParserError> parse(std::string_view smartctl_output) = 0;


		/// Detect smartctl output type (text, json).
		[[nodiscard]] static hz::ExpectedValue<SmartctlOutputFormat, SmartctlParserError> detect_output_format(std::string_view smartctl_output);


		/// Get parsed properties.
		[[nodiscard]] const StoragePropertyRepository& get_property_repository() const;


	protected:

		/// Add a property into property list, look up and set its description
		void add_property(AtaStorageProperty p);


	private:

		StoragePropertyRepository properties_;  ///< Parsed data properties

};




#endif

/// @}
