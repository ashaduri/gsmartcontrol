/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef SMARTCTL_EXECUTOR_GUI_H
#define SMARTCTL_EXECUTOR_GUI_H

#include "hz/intrusive_ptr.h"

#include "smartctl_executor.h"
#include "cmdex_sync_gui.h"



typedef SmartctlExecutorGeneric<CmdexSyncGui> SmartctlExecutorGui;


typedef hz::intrusive_ptr<SmartctlExecutorGui> SmartctlExecutorGuiRefPtr;




#endif
