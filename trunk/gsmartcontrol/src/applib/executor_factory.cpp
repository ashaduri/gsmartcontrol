/**************************************************************************
 Copyright:
      (C) 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
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
#include "tw_cli_executor.h"




ExecutorFactory::ExecutorFactory(bool use_gui, Gtk::Window* parent)
		: use_gui_(use_gui), parent_(parent)
{ }



hz::intrusive_ptr<CmdexSync> ExecutorFactory::create_executor(ExecutorFactory::Type type)
{
	switch (type) {
		case ExecutorSmartctl:
		{
			if (use_gui_) {
				SmartctlExecutorGuiRefPtr ex = SmartctlExecutorGuiRefPtr(new SmartctlExecutorGui());
				ex->create_running_dialog(parent_);  // dialog parent
				return ex;
			}
			return SmartctlExecutorRefPtr(new SmartctlExecutor());
		}
		case ExecutorTwCli:
		{
			if (use_gui_) {
				TwCliExecutorGuiRefPtr ex = TwCliExecutorGuiRefPtr(new TwCliExecutorGui());
				ex->create_running_dialog(parent_);  // dialog parent
				return ex;
			}
			return TwCliExecutorRefPtr(new TwCliExecutor());
		}
	}

	DBG_ASSERT(0);
	return hz::intrusive_ptr<CmdexSync>();
}






/// @}
