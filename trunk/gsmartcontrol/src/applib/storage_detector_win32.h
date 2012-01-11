/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef STORAGE_DETECTOR_WIN32_H
#define STORAGE_DETECTOR_WIN32_H

#include <string>
#include <vector>

#include "hz/hz_config.h"  // CONFIG_*
#include "executor_factory.h"
#include "storage_device.h"


#if defined CONFIG_KERNEL_FAMILY_WINDOWS


/// Detect drives in Windows
std::string detect_drives_win32(std::vector<StorageDeviceRefPtr>& drives, ExecutorFactoryRefPtr ex_factory);


#endif


#endif

/// @}
