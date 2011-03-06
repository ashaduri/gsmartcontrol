/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef STORAGE_DETECTOR_LINUX_H
#define STORAGE_DETECTOR_LINUX_H

#include <string>
#include <vector>

#include "hz/hz_config.h"  // CONFIG_*


#if defined CONFIG_KERNEL_LINUX


/// Detect drives in linux using udev
// std::string detect_drives_linux_udev_byid(std::vector<std::string>& devices);


/// Detect drives in linux using /proc/partitions
std::string detect_drives_linux_proc_partitions(std::vector<std::string>& devices);


#endif


#endif
