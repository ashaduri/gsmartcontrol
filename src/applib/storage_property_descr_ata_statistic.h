/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2024 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef STORAGE_PROPERTY_DESCR_ATA_STATISTIC_H
#define STORAGE_PROPERTY_DESCR_ATA_STATISTIC_H

#include "storage_property_repository.h"
//#include "storage_device_detected_type.h"



/// Find a property's statistic in the statistic database and fill the property
/// with all the readable information we can gather.
bool auto_set_ata_statistic_description(StorageProperty& p);


/// If p is of appropriate type, set the warning on it if needed.
void storage_property_ata_statistic_autoset_warning(StorageProperty& p);


#endif

/// @}
