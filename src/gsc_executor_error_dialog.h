/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gsc
/// \weakgroup gsc
/// @{

#ifndef GSC_EXECUTOR_ERROR_DIALOG_H
#define GSC_EXECUTOR_ERROR_DIALOG_H

#include <string>
#include <gtkmm.h>



/// Show a dialog when an execution error occurs. A dialog
/// will have a "Show Output" button, which shows the last executed
/// command details.
void gsc_executor_error_dialog_show(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent, bool sec_msg_markup = false, bool show_output_button = true);


/// Show a dialog when no additional information is available.
/// If \c output is not empty, a "Show Output" button will be displayed
/// which shows this output.
void gsc_no_info_dialog_show(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent, bool sec_msg_markup, const std::string& output,
		const std::string& output_window_title, const std::string& default_save_filename);




#endif

/// @}
