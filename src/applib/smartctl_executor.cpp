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

#include "local_glibmm.h"

#include "smartctl_executor.h"
#include "hz/win32_tools.h"
#include "rconfig/rconfig.h"
#include "app_regex.h"
#include "hz/fs.h"
#include "build_config.h"
#include <vector>



hz::fs::path get_smartctl_binary()
{
	auto smartctl_binary = hz::fs_path_from_string(rconfig::get_data<std::string>("system/smartctl_binary"));

	if (BuildEnv::is_kernel_family_windows()) {
		// Look in smartmontools installation directory.
		hz::fs::path system_binary;
		do {
			const bool use_smt = rconfig::get_data<bool>("system/win32_search_smartctl_in_smartmontools");
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

			auto p = hz::fs_path_from_string(smt_inst_dir) / hz::fs_path_from_string(smt_smartctl);

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



hz::ExpectedVoid<SmartctlExecutorError> execute_smartctl(const std::string& device, const std::vector<std::string>& device_opts,
		const std::vector<std::string>& command_options,
		std::shared_ptr<CommandExecutor> smartctl_ex, std::string& smartctl_output)
{
	// win32 doesn't have slashes in devices names. For others, check that slash is present.
	if (!BuildEnv::is_kernel_family_windows()) {
		const std::string::size_type pos = device.rfind('/');  // find basename
		if (pos == std::string::npos) {
			debug_out_error("app", DBG_FUNC_MSG << "Invalid device name \"" << device << "\".\n");
			return hz::Unexpected(SmartctlExecutorError::InvalidDevice, _("Invalid device name specified."));
		}
	}


	if (!smartctl_ex)  // if it doesn't exist, create a default one
		smartctl_ex = std::make_shared<SmartctlExecutor>();

	auto smartctl_binary = get_smartctl_binary();

	if (smartctl_binary.empty()) {
		debug_out_error("app", DBG_FUNC_MSG << "Smartctl binary is not set in config.\n");
		return hz::Unexpected(SmartctlExecutorError::NoBinary, _("Smartctl binary is not specified in configuration."));
	}

	auto smartctl_def_options_str = hz::string_trim_copy(rconfig::get_data<std::string>("system/smartctl_options"));
	std::vector<std::string> smartctl_options;
	if (!smartctl_def_options_str.empty()) {
		try {
			smartctl_options = Glib::shell_parse_argv(smartctl_def_options_str);
		}
		catch(Glib::ShellError& e)
		{
			return hz::Unexpected(SmartctlExecutorError::InvalidCommandLine, _("Invalid command line specified."));
		}
	}
	smartctl_options.insert(smartctl_options.end(), device_opts.begin(), device_opts.end());
	smartctl_options.insert(smartctl_options.end(), command_options.begin(), command_options.end());
	smartctl_options.push_back(device);

	smartctl_ex->set_command(hz::fs_path_to_string(smartctl_binary), smartctl_options);

	if (!smartctl_ex->execute() || !smartctl_ex->get_error_msg().empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Smartctl binary did not execute cleanly.\n");

		smartctl_output = hz::string_trim_copy(hz::string_any_to_unix_copy(smartctl_ex->get_stdout_str()));

		// check if it's a device permission error.
		// Smartctl open device: /dev/sdb failed: Permission denied
		if (app_regex_partial_match("/Smartctl open device.+Permission denied/mi", smartctl_output)) {
			return hz::Unexpected(SmartctlExecutorError::PermissionDenied, _("Permission denied while opening device."));
		}

		// smartctl_output = smartctl_ex->get_stdout_str();
		return hz::Unexpected(SmartctlExecutorError::ExecutionError, smartctl_ex->get_error_msg());
	}

	// any_to_unix is needed for windows
	smartctl_output = hz::string_trim_copy(hz::string_any_to_unix_copy(smartctl_ex->get_stdout_str()));
	if (smartctl_output.empty()) {
		debug_out_error("app", DBG_FUNC_MSG << "Smartctl returned an empty output.\n");
		return hz::Unexpected(SmartctlExecutorError::EmptyOutput, _("Smartctl returned an empty output."));
	}

	return {};
}




/// @}
