/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef STORAGE_DETECTOR_WIN32_H
#define STORAGE_DETECTOR_WIN32_H

#include <string>
#include <vector>

#include "hz/hz_config.h"  // CONFIG_*


#if defined CONFIG_KERNEL_FAMILY_WINDOWS


std::string detect_drives_win32(std::vector<std::string>& devices);


#endif


#endif
