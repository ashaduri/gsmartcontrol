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

#ifndef STORAGE_PROPERTY_DESCR_ATA_ATTRIBUTE_H
#define STORAGE_PROPERTY_DESCR_ATA_ATTRIBUTE_H

//#include "storage_property_repository.h"
#include "storage_device_detected_type.h"
#include "storage_property.h"


/// Find a property's attribute in the attribute database and fill the property
/// with all the readable information we can gather.
void auto_set_ata_attribute_description(StorageProperty& p, StorageDeviceDetectedType drive_type);


/// If p is of appropriate type, set the warning on it if needed.
void storage_property_ata_attribute_autoset_warning(StorageProperty& p);


#endif

/// @}
