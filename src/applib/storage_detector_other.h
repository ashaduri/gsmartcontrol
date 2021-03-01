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

#ifndef STORAGE_DETECTOR_OTHER_H
#define STORAGE_DETECTOR_OTHER_H

#include "build_config.h"  // CONFIG_*

#if !defined CONFIG_KERNEL_LINUX && !defined CONFIG_KERNEL_FAMILY_WINDOWS


#include <string>
#include <vector>

#include "executor_factory.h"
#include "storage_device.h"



/// Detect drives in FreeBSD, Solaris, etc... (all except Linux and Windows).
std::string detect_drives_other(std::vector<StorageDevicePtr>& drives, const ExecutorFactoryPtr& ex_factory);



#endif

#endif

/// @}
