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

#ifndef STORAGE_DETECTOR_WIN32_H
#define STORAGE_DETECTOR_WIN32_H

#include "config.h"  // CONFIG_*

#if defined CONFIG_KERNEL_FAMILY_WINDOWS


#include <string>
#include <vector>

#include "executor_factory.h"
#include "storage_device.h"



/// Detect drives in Windows
std::string detect_drives_win32(std::vector<StorageDevicePtr>& drives, const ExecutorFactoryPtr& ex_factory);



#endif

#endif

/// @}
