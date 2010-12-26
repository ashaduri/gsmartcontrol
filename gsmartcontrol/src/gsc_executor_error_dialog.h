/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef GSC_EXECUTOR_ERROR_DIALOG_H
#define GSC_EXECUTOR_ERROR_DIALOG_H

#include <string>
#include <gtkmm/window.h>




void gsc_executor_error_dialog_show(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent, bool show_output_button = true, bool sec_msg_markup = false);




#endif
