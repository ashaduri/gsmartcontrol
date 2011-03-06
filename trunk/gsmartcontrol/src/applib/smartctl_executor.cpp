/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include "smartctl_executor.h"

#include "hz/fs_path.h"
#include "hz/win32_tools.h"
#include "rconfig/rconfig_mini.h"



// returns an empty string if not found.
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



