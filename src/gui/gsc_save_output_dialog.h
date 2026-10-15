/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2026 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gsc
/// \weakgroup gsc
/// @{

#ifndef GSC_SAVE_OUTPUT_DIALOG_H
#define GSC_SAVE_OUTPUT_DIALOG_H

#include <gtkmm.h>
#include <string>

#include "applib/smartctl_parser_types.h"



/// Show a "Save As" dialog for smartctl output, and save it in the format chosen by the user
/// (JSON or text, depending on availability). The format is chosen by the selected file type,
/// or by the file extension if the file type is not format-specific.
/// \param parent Parent window
/// \param smartctl_output Smartctl output (JSON or text)
/// \param default_filename Suggested file name. Its extension is adjusted to match \c preferred_format.
/// \param preferred_format Format to suggest, if available
void gsc_show_save_smartctl_output_dialog(Gtk::Window* parent, const std::string& smartctl_output,
		const std::string& default_filename, SmartctlOutputFormat preferred_format);



#endif

/// @}
