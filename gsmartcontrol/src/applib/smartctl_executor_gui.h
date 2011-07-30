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

#ifndef SMARTCTL_EXECUTOR_GUI_H
#define SMARTCTL_EXECUTOR_GUI_H

#include "hz/intrusive_ptr.h"

#include "smartctl_executor.h"
#include "cmdex_sync_gui.h"



/// Smartctl executor with GUI support
typedef SmartctlExecutorGeneric<CmdexSyncGui> SmartctlExecutorGui;


/// A reference-counting pointer to SmartctlExecutor
typedef hz::intrusive_ptr<SmartctlExecutorGui> SmartctlExecutorGuiRefPtr;




#endif

/// @}
