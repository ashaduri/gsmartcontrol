/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include <glibmm.h>  // Glib::shell_quote()

#include "smartctl_executor.h"

#include "hz/fs_path.h"
#include "hz/win32_tools.h"
#include "rconfig/rconfig_mini.h"
#include "app_pcrecpp.h"




std::string get_smartctl_binary()
{
	std::string smartctl_binary;
	rconfig::get_data("system/smartctl_binary", smartctl_binary);

#ifdef _WIN32
	// look in smartmontools installation directory.
	do {
		bool use_smt = false;
		rconfig::get_data("system/win32_search_smartctl_in_smartmontools", use_smt);
		if (!use_smt)
			break;

		std::string smt_regpath, smt_regkey, smt_smartctl;
		rconfig::get_data("system/win32_smartmontools_regpath", smt_regpath);
		rconfig::get_data("system/win32_smartmontools_regkey", smt_regkey);
		rconfig::get_data("system/win32_smartmontools_smartctl_binary", smt_smartctl);

		if (smt_regpath.empty() || smt_regkey.empty() || smt_smartctl.empty())
			break;

		std::string smt_inst_dir;
		hz::win32_get_registry_value_string(HKEY_LOCAL_MACHINE, smt_regpath, smt_regkey, smt_inst_dir);

		if (smt_inst_dir.empty()) {
			debug_out_info("app", DBG_FUNC_MSG << "Smartmontools installation not found in \"HKLM\\"
					<< smt_regpath << "\\" << smt_regkey << "\".\n");
			break;
		}
		debug_out_info("app", DBG_FUNC_MSG << "Smartmontools installation found at \"" << smt_inst_dir
				<< "\", using \"" << smt_smartctl << "\".\n");

		hz::FsPath p(smt_inst_dir);
		p.append(smt_smartctl);

		if (!p.exists() || !p.is_file())
			break;

		smartctl_binary = p.str();

	} while (false);

#endif

	return smartctl_binary;
}



std::string execute_smartctl(const std::string& device, const std::string& device_opts,
		const std::string& command_options,
		hz::intrusive_ptr<CmdexSync> smartctl_ex, std::string& smartctl_output)
{
#ifndef _WIN32  // win32 doesn't have slashes in devices names
	{
		std::string::size_type pos = device.rfind('/');  // find basename
		if (pos == std::string::npos) {
			debug_out_error("app", DBG_FUNC_MSG << "Invalid device name \"" << device << "\".\n");
			return "Invalid device name specified.";
		}
	}
#endif

	if (!smartctl_ex)  // if it doesn't exist, create a default one
		smartctl_ex = new SmartctlExecutor();  // will be auto-deleted

	std::string smartctl_binary = get_smartctl_binary();

	if (smartctl_binary.empty()) {
		debug_out_error("app", DBG_FUNC_MSG << "Smartctl binary is not set in config.\n");
		return "Smartctl binary is not specified in configuration.";
	}

	std::string smartctl_def_options;
	rconfig::get_data("system/smartctl_options", smartctl_def_options);

	if (!smartctl_def_options.empty())
		smartctl_def_options += " ";


	std::string device_specific_options = device_opts;
	if (!device_specific_options.empty())
		device_specific_options += " ";


	smartctl_ex->set_command(Glib::shell_quote(smartctl_binary),
			smartctl_def_options + device_specific_options + command_options
			+ " " + Glib::shell_quote(device));

	if (!smartctl_ex->execute() || !smartctl_ex->get_error_msg().empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Error while executing smartctl binary.\n");

		smartctl_output = hz::string_trim_copy(hz::string_any_to_unix_copy(smartctl_ex->get_stdout_str()));

		// check if it's a device permission error.
		// Smartctl open device: /dev/sdb failed: Permission denied
		if (app_pcre_match("/Smartctl open device.+Permission denied/mi", smartctl_output)) {
			return "Permission denied while opening device.";
		}

		// smartctl_output = smartctl_ex->get_stdout_str();
		return smartctl_ex->get_error_msg();
	}

	// any_to_unix is needed for windows
	smartctl_output = hz::string_trim_copy(hz::string_any_to_unix_copy(smartctl_ex->get_stdout_str()));
	if (smartctl_output.empty()) {
		debug_out_error("app", DBG_FUNC_MSG << "Smartctl returned an empty output.\n");
		return "Smartctl returned an empty output.";
	}

	return std::string();
}




/// @}
