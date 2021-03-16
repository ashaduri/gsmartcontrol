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

#ifndef ATA_STORAGE_PROPERTY_DESCR_H
#define ATA_STORAGE_PROPERTY_DESCR_H

#include "ata_storage_property.h"



/// Fill the property with all the information we can gather (description, etc...).
bool ata_storage_property_autoset_description(AtaStorageProperty& p, AtaStorageAttribute::DiskType disk_type);


/// Do some basic checks on the property and set warnings if needed.
WarningLevel ata_storage_property_autoset_warning(AtaStorageProperty& p);



#endif

/// @}
