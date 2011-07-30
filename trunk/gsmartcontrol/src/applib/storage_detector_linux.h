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

#ifndef STORAGE_DETECTOR_LINUX_H
#define STORAGE_DETECTOR_LINUX_H

#include <string>
#include <vector>

#include "hz/hz_config.h"  // CONFIG_*
#include "executor_factory.h"
#include "storage_device.h"


#if defined CONFIG_KERNEL_LINUX



/// Detect drives in Linux
std::string detect_drives_linux(std::vector<StorageDeviceRefPtr>& drives, ExecutorFactoryRefPtr ex_factory);



#endif


#endif

/// @}
