/**************************************************************************
 Copyright:
      (C) 2011 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include "hz/debug.h"
#include "executor_factory.h"
#include "smartctl_executor_gui.h"
#include "cli_executors.h"




ExecutorFactory::ExecutorFactory(bool use_gui, Gtk::Window* parent)
		: use_gui_(use_gui), parent_(parent)
{ }



std::shared_ptr<CmdexSync> ExecutorFactory::create_executor(ExecutorFactory::ExecutorType type)
{
	switch (type) {
		case ExecutorType::Smartctl:
		{
			if (use_gui_) {
				auto ex = std::make_shared<SmartctlExecutorGui>();
				ex->create_running_dialog(parent_);  // dialog parent
				return ex;
			}
			return std::make_shared<SmartctlExecutor>();
		}
		case ExecutorType::TwCli:
		{
			if (use_gui_) {
				auto ex = std::make_shared<TwCliExecutorGui>();
				ex->create_running_dialog(parent_);  // dialog parent
				return ex;
			}
			return std::make_shared<TwCliExecutor>();
		}
		case ExecutorType::ArecaCli:
		{
			if (use_gui_) {
				auto ex = std::make_shared<ArecaCliExecutorGui>();
				ex->create_running_dialog(parent_);  // dialog parent
				return ex;
			}
			return std::make_shared<ArecaCliExecutor>();
		}
	}

	DBG_ASSERT(0);
	return std::make_shared<CmdexSync>();
}






/// @}
