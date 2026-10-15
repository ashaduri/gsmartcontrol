/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2026 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include "smartctl_output_export.h"

#include <vector>

#include "nlohmann/json.hpp"
#include "hz/debug.h"
#include "hz/string_algo.h"  // string_join
#include "smartctl_parser.h"



bool SmartctlOutputVariants::has(SmartctlOutputFormat format) const
{
	return (format == SmartctlOutputFormat::Json ? json : text).has_value();
}



const std::string& SmartctlOutputVariants::get(SmartctlOutputFormat format) const
{
	return (format == SmartctlOutputFormat::Json ? json : text).value();
}



SmartctlOutputVariants smartctl_output_get_variants(std::string_view smartctl_output)
{
	SmartctlOutputVariants variants;

	const auto format = SmartctlParser::detect_output_format(smartctl_output);
	if (!format.has_value() || format.value() != SmartctlOutputFormat::Json) {
		// Text output, or something unknown (e.g. an error message), which is text as well.
		variants.text = std::string(smartctl_output);
		return variants;
	}

	variants.json = std::string(smartctl_output);

	try {
		const nlohmann::json root_node = nlohmann::json::parse(smartctl_output);
		const auto smartctl_iter = root_node.find("smartctl");
		if (smartctl_iter != root_node.end() && smartctl_iter->is_object()) {
			const auto output_iter = smartctl_iter->find("output");
			if (output_iter != smartctl_iter->end() && output_iter->is_array() && !output_iter->empty()) {
				std::vector<std::string> lines;
				for (const auto& entry : *output_iter) {
					if (entry.is_string()) {
						lines.emplace_back(entry.get<std::string>());
					}
				}
				variants.text = hz::string_join(lines, "\n") + "\n";
			}
		}
	} catch (const nlohmann::json::exception& e) {
		debug_out_warn("app", DBG_FUNC_MSG << "Cannot extract text output from JSON: " << e.what() << "\n");
	}

	return variants;
}



std::string smartctl_output_get_file_extension(SmartctlOutputFormat format)
{
	return format == SmartctlOutputFormat::Json ? ".json" : ".txt";
}



hz::fs::path smartctl_output_set_file_extension(hz::fs::path file, SmartctlOutputFormat format)
{
	const std::string ext = smartctl_output_get_file_extension(format);
	if (file.extension() == ".json" || file.extension() == ".txt") {
		file.replace_extension(ext);
	} else {
		file += ext;
	}
	return file;
}



std::pair<hz::fs::path, SmartctlOutputFormat> smartctl_output_resolve_save_target(hz::fs::path file,
		std::optional<SmartctlOutputFormat> filter_format, SmartctlOutputFormat default_format,
		const SmartctlOutputVariants& variants)
{
	SmartctlOutputFormat format = default_format;
	if (filter_format.has_value()) {
		format = filter_format.value();
	} else if (file.extension() == ".txt") {
		format = SmartctlOutputFormat::Text;
	} else if (file.extension() == ".json") {
		format = SmartctlOutputFormat::Json;
	}

	if (!variants.has(format)) {
		format = (format == SmartctlOutputFormat::Json ? SmartctlOutputFormat::Text : SmartctlOutputFormat::Json);
	}

	return {smartctl_output_set_file_extension(std::move(file), format), format};
}



/// @}
