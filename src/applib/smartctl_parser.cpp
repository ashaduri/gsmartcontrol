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
#include "smartctl_ata_text_parser.h"
#include "smartctl_ata_json_parser.h"
#include "ata_storage_property_descr.h"
#include "warning_colors.h"



std::unique_ptr<SmartctlParser> SmartctlParser::create(SmartctlParserType type)
{
	switch(type) {
		case SmartctlParserType::Json:
			return std::make_unique<SmartctlAtaJsonParser>();
		case SmartctlParserType::Text:
			return std::make_unique<SmartctlAtaTextParser>();
	}
	return nullptr;
}



// std::optional<SmartctlParserType> SmartctlParser::detect_output_type(const std::string& output) const
// {
	// Look for the first non-whitespace symbol
	// auto first_symbol = std::find_if(output.begin(), output.end(), [&](char c) {
	// 	return !std::isspace(c, std::locale::classic());
	// });
	// if (first_symbol != output.end() && *first_symbol == '-'




// }



std::string SmartctlParser::get_data_full() const
{
	return data_full_;
}



std::string SmartctlParser::get_error_msg() const
{
	return Glib::ustring::compose(_("Cannot parse smartctl output: %1"), error_msg_);
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



void SmartctlParser::set_data_full(const std::string& s)
{
	data_full_ = s;
}



void SmartctlParser::set_error_msg(const std::string& s)
{
	error_msg_ = s;
}







/// @}
