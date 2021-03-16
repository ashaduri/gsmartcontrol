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

#include "hz/debug.h"
#include "command_executor_factory.h"
#include "smartctl_executor_gui.h"
#include "command_executor_areca.h"
#include "command_executor_3ware.h"



CommandExecutorFactory::CommandExecutorFactory(bool use_gui, Gtk::Window* parent)
		: use_gui_(use_gui), parent_(parent)
{ }



std::shared_ptr<CommandExecutor> CommandExecutorFactory::create_executor(CommandExecutorFactory::ExecutorType type)
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
	return std::make_shared<CommandExecutor>();
}






/// @}
