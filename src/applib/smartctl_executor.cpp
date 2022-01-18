/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include "local_glibmm.h"  // Glib::shell_quote()

#include "smartctl_executor.h"
#include "hz/win32_tools.h"
#include "rconfig/rconfig.h"
#include "app_pcrecpp.h"
#include "hz/fs.h"
#include "build_config.h"



hz::fs::path get_smartctl_binary()
{
	auto smartctl_binary = hz::fs::u8path(rconfig::get_data<std::string>("system/smartctl_binary"));

	if constexpr(BuildEnv::is_kernel_family_windows()) {
		// Look in smartmontools installation directory.
		hz::fs::path system_binary;
		do {
			bool use_smt = rconfig::get_data<bool>("system/win32_search_smartctl_in_smartmontools");
			if (!use_smt)
				break;

			auto smt_regpath = rconfig::get_data<std::string>("system/win32_smartmontools_regpath");
			auto smt_regpath_wow = rconfig::get_data<std::string>("system/win32_smartmontools_regpath_wow");  // same as above, but with WOW6432Node
			auto smt_regkey = rconfig::get_data<std::string>("system/win32_smartmontools_regkey");
			auto smt_smartctl = rconfig::get_data<std::string>("system/win32_smartmontools_smartctl_binary");

			if ((smt_regpath.empty() && smt_regpath_wow.empty()) || smt_regkey.empty() || smt_smartctl.empty())
				break;

			std::string smt_inst_dir;
			#ifdef _WIN32
				hz::win32_get_registry_value_string(HKEY_LOCAL_MACHINE, smt_regpath, smt_regkey, smt_inst_dir);
				if (smt_inst_dir.empty()) {
					hz::win32_get_registry_value_string(HKEY_LOCAL_MACHINE, smt_regpath_wow, smt_regkey, smt_inst_dir);
				}
			#endif

			if (smt_inst_dir.empty()) {
				debug_out_info("app", DBG_FUNC_MSG << "Smartmontools installation not found in \"HKLM\\"
						<< smt_regpath << "\\" << smt_regkey << "\".\n");
				break;
			}
			debug_out_info("app", DBG_FUNC_MSG << "Smartmontools installation found at \"" << smt_inst_dir
					<< "\", using \"" << smt_smartctl << "\".\n");

			auto p = hz::fs::u8path(smt_inst_dir) / hz::fs::u8path(smt_smartctl);

			if (!hz::fs::exists(p) || !hz::fs::is_regular_file(p))
				break;

			system_binary = p;
		} while (false);

		if (!system_binary.empty()) {
			smartctl_binary = system_binary;

		} else if (smartctl_binary.is_relative()) {
			// If smartctl path is relative, and it's Windows, and the package seems to contain smartctl, use our own binary.
			if (auto app_dir = hz::fs_get_application_dir(); !app_dir.empty() && hz::fs::exists(app_dir / smartctl_binary)) {
				smartctl_binary = app_dir / smartctl_binary;
			}
		}
	}

	return smartctl_binary;
}



std::string execute_smartctl(const std::string& device, const std::string& device_opts,
		const std::string& command_options,
		std::shared_ptr<CommandExecutor> smartctl_ex, std::string& smartctl_output)
{
	// win32 doesn't have slashes in devices names. For others, check that slash is present.
	if constexpr(!BuildEnv::is_kernel_family_windows()) {
		std::string::size_type pos = device.rfind('/');  // find basename
		if (pos == std::string::npos) {
			debug_out_error("app", DBG_FUNC_MSG << "Invalid device name \"" << device << "\".\n");
			return _("Invalid device name specified.");
		}
	}


	if (!smartctl_ex)  // if it doesn't exist, create a default one
		smartctl_ex = std::make_shared<SmartctlExecutor>();

	auto smartctl_binary = get_smartctl_binary();

	if (smartctl_binary.empty()) {
		debug_out_error("app", DBG_FUNC_MSG << "Smartctl binary is not set in config.\n");
		return _("Smartctl binary is not specified in configuration.");
	}

	auto smartctl_def_options = rconfig::get_data<std::string>("system/smartctl_options");

	if (!smartctl_def_options.empty())
		smartctl_def_options += " ";


	std::string device_specific_options = device_opts;
	if (!device_specific_options.empty())
		device_specific_options += " ";


	smartctl_ex->set_command(Glib::shell_quote(smartctl_binary.u8string()),
			smartctl_def_options + device_specific_options + command_options
			+ " " + Glib::shell_quote(device));

	if (!smartctl_ex->execute() || !smartctl_ex->get_error_msg().empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Smartctl binary did not execute cleanly.\n");

		smartctl_output = hz::string_trim_copy(hz::string_any_to_unix_copy(smartctl_ex->get_stdout_str()));

		// check if it's a device permission error.
		// Smartctl open device: /dev/sdb failed: Permission denied
		if (app_pcre_match("/Smartctl open device.+Permission denied/mi", smartctl_output)) {
			return _("Permission denied while opening device.");
		}

		// smartctl_output = smartctl_ex->get_stdout_str();
		return smartctl_ex->get_error_msg();
	}

	// any_to_unix is needed for windows
	smartctl_output = hz::string_trim_copy(hz::string_any_to_unix_copy(smartctl_ex->get_stdout_str()));
	if (smartctl_output.empty()) {
		debug_out_error("app", DBG_FUNC_MSG << "Smartctl returned an empty output.\n");
		return _("Smartctl returned an empty output.");
	}

	return {};
}




/// @}
