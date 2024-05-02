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
#include <utility>
#include <string_view>

#include "smartctl_parser.h"
#include "ata_storage_property.h"
#include "hz/error_container.h"
#include "smartctl_text_ata_parser.h"
#include "smartctl_json_ata_parser.h"
#include "smartctl_json_basic_parser.h"
#include "smartctl_text_basic_parser.h"
#include "storage_property_repository.h"
#include "smartctl_json_nvme_parser.h"
//#include "ata_storage_property_descr.h"



std::unique_ptr<SmartctlParser> SmartctlParser::create(SmartctlParserType type, SmartctlOutputFormat format)
{
	switch(type) {
		case SmartctlParserType::Basic:
			switch(format) {
				case SmartctlOutputFormat::Json:
					return std::make_unique<SmartctlJsonBasicParser>();
					break;
				case SmartctlOutputFormat::Text:
					return std::make_unique<SmartctlTextBasicParser>();
					break;
			}
			break;
		case SmartctlParserType::Ata:
			switch(format) {
				case SmartctlOutputFormat::Json:
					return std::make_unique<SmartctlJsonAtaParser>();
				case SmartctlOutputFormat::Text:
					return std::make_unique<SmartctlTextAtaParser>();
			}
			break;
		case SmartctlParserType::Nvme:
			switch(format) {
				case SmartctlOutputFormat::Json:
					return std::make_unique<SmartctlJsonNvmeParser>();
				case SmartctlOutputFormat::Text:
					// nothing
					break;
			}
			break;
	}
	return nullptr;
}



hz::ExpectedValue<SmartctlOutputFormat, SmartctlParserError> SmartctlParser::detect_output_format(std::string_view smartctl_output)
{
	// Look for the first non-whitespace symbol
	const auto* first_symbol = std::find_if(smartctl_output.begin(), smartctl_output.end(), [&](char c) {
		return !std::isspace(c, std::locale::classic());
	});
	if (first_symbol != smartctl_output.end()) {
		if (*first_symbol == '{') {
			return SmartctlOutputFormat::Json;
		}
		if (smartctl_output.rfind("smartctl", static_cast<std::size_t>(first_symbol - smartctl_output.begin())) == 0) {
			return SmartctlOutputFormat::Text;
		}
		return hz::Unexpected(SmartctlParserError::UnsupportedFormat, "Unsupported format while trying to detect smartctl output format.");
	}
	return hz::Unexpected(SmartctlParserError::EmptyInput, "Empty input while trying to detect smartctl output format.");
}



const StoragePropertyRepository& SmartctlParser::get_property_repository() const
{
	return properties_;
}



// adds a property into property list, looks up and sets its description.
// Yes, there's no place for this in the Parser, but whatever...
void SmartctlParser::add_property(AtaStorageProperty p)
{
	properties_.add_property(std::move(p));
}





/// @}
