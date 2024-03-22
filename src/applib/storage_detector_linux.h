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

#ifndef STORAGE_DETECTOR_LINUX_H
#define STORAGE_DETECTOR_LINUX_H

#include "build_config.h"


#include <string>
#include <vector>

#include "command_executor_factory.h"
#include "storage_device.h"
#include "storage_detector.h"



/// Detect drives in Linux
[[nodiscard]] hz::ExpectedVoid<StorageDetectorError> detect_drives_linux(std::vector<StorageDevicePtr>& drives,
		const CommandExecutorFactoryPtr& ex_factory);




#endif

/// @}
