/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef SMARTCTL_EXECUTOR_GUI_H
#define SMARTCTL_EXECUTOR_GUI_H

#include "hz/intrusive_ptr.h"

#include "smartctl_executor.h"
#include "cmdex_sync_gui.h"



/// Smartctl executor with GUI support
using SmartctlExecutorGui = SmartctlExecutorGeneric<CmdexSyncGui>;


/// A reference-counting pointer to SmartctlExecutor
using SmartctlExecutorGuiRefPtr = hz::intrusive_ptr<SmartctlExecutorGui>;




#endif

/// @}
