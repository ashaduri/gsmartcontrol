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

#ifndef STORAGE_PROPERTY_DESCR_H
#define STORAGE_PROPERTY_DESCR_H

#include "storage_property.h"



/// Fill the property with all the information we can gather (description, etc...).
bool storage_property_autoset_description(StorageProperty& p, StorageAttribute::DiskType disk_type);


/// Do some basic checks on the property and set warnings if needed.
WarningLevel storage_property_autoset_warning(StorageProperty& p);



#endif

/// @}
