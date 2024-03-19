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

#include <locale>
#include <cctype>  // isspace

#include "smartctl_parser.h"
#include "smartctl_text_ata_parser.h"
#include "smartctl_json_ata_parser.h"
#include "smartctl_json_basic_parser.h"
#include "smartctl_text_basic_parser.h"
//#include "ata_storage_property_descr.h"



std::unique_ptr<SmartctlParser> SmartctlParser::create(SmartctlParserType type)
{
	switch(type) {
		case SmartctlParserType::JsonBasic:
			return std::make_unique<SmartctlJsonBasicParser>();
			break;
		case SmartctlParserType::JsonAta:
			return std::make_unique<SmartctlJsonAtaParser>();
			break;
		case SmartctlParserType::TextBasic:
			return std::make_unique<SmartctlTextBasicParser>();
			break;
		case SmartctlParserType::TextAta:
			return std::make_unique<SmartctlTextAtaParser>();
			break;
	}
	return nullptr;
}



hz::ExpectedValue<SmartctlParserFormat, SmartctlParserError> SmartctlParser::detect_output_format(std::string_view smartctl_output)
{
	// Look for the first non-whitespace symbol
	const auto* first_symbol = std::find_if(smartctl_output.begin(), smartctl_output.end(), [&](char c) {
		return !std::isspace(c, std::locale::classic());
	});
	if (first_symbol != smartctl_output.end()) {
		if (*first_symbol == '{') {
			return SmartctlParserFormat::Json;
		}
		if (smartctl_output.rfind("smartctl", static_cast<std::size_t>(first_symbol - smartctl_output.begin())) == 0) {
			return SmartctlParserFormat::Text;
		}
		return hz::Unexpected(SmartctlParserError::UnsupportedFormat, "Unsupported format while trying to detect smartctl output format.");
	}
	return hz::Unexpected(SmartctlParserError::EmptyInput, "Empty input while trying to detect smartctl output format.");
}



const std::vector<AtaStorageProperty>& SmartctlParser::get_properties() const
{
	return properties_;
}



// adds a property into property list, looks up and sets its description.
// Yes, there's no place for this in the Parser, but whatever...
void SmartctlParser::add_property(AtaStorageProperty p)
{
	properties_.push_back(std::move(p));
}





/// @}
