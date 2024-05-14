/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2022 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef SMARTCTL_TEXT_PARSER_HELPER_H
#define SMARTCTL_TEXT_PARSER_HELPER_H

#include <string>
#include <cstdint>



/// Helpers for smartctl text output parser
class SmartctlTextParserHelper {
	public:

		/// Convert e.g. "1,000,204,886,016 bytes" to "1.00 TiB [931.51 GB, 1000204886016 bytes]"
		/// \param str String to parse
		/// \param bytes Number of bytes
		/// \param extended Return size in other units as well
		/// \return Size as a displayable string
		static std::string parse_byte_size(std::string str, int64_t& bytes, bool extended);

};






#endif

/// @}
