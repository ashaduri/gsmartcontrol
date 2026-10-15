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

#ifndef SMARTCTL_OUTPUT_EXPORT_H
#define SMARTCTL_OUTPUT_EXPORT_H

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "hz/fs.h"
#include "smartctl_parser_types.h"



/// Smartctl output, available in JSON and / or text formats.
struct SmartctlOutputVariants {
	std::optional<std::string> json;  ///< JSON output, if available
	std::optional<std::string> text;  ///< Text (non-JSON) output, if available

	/// Check if the output is available in the specified format
	[[nodiscard]] bool has(SmartctlOutputFormat format) const;

	/// Get the output in the specified format. The format must be available.
	[[nodiscard]] const std::string& get(SmartctlOutputFormat format) const;
};



/// Get smartctl output in all the available formats.
/// JSON output contains the text output only if it was obtained with "--json=o" (which we always use);
/// text output is available as is, and has no JSON version.
[[nodiscard]] SmartctlOutputVariants smartctl_output_get_variants(std::string_view smartctl_output);



/// Get the default file extension (including the dot) for the format.
[[nodiscard]] std::string smartctl_output_get_file_extension(SmartctlOutputFormat format);



/// Replace the file name extension with the one corresponding to the format
/// if it's ".json" or ".txt", or append it otherwise.
[[nodiscard]] hz::fs::path smartctl_output_set_file_extension(hz::fs::path file, SmartctlOutputFormat format);



/// Decide which format to save the smartctl output in, and adjust the file name extension accordingly.
/// \param file File name chosen by the user
/// \param filter_format Format of the file filter selected by the user, if the filter is format-specific
/// \param default_format Format to use if neither the filter nor the extension determine it
/// \param variants Available output formats. At least one must be available.
/// \return The file name with the correct extension, and the format to save in.
[[nodiscard]] std::pair<hz::fs::path, SmartctlOutputFormat> smartctl_output_resolve_save_target(hz::fs::path file,
		std::optional<SmartctlOutputFormat> filter_format, SmartctlOutputFormat default_format,
		const SmartctlOutputVariants& variants);



#endif

/// @}
