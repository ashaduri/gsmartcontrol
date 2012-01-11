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

#ifndef STORAGE_PROPERTY_DESCR_H
#define STORAGE_PROPERTY_DESCR_H

#include "storage_property.h"



/// Fill the property with all the information we can gather (description, etc...).
bool storage_property_autoset_description(StorageProperty& p);


/// Do some basic checks on the property and set warnings if needed.
StorageProperty::warning_t storage_property_autoset_warning(StorageProperty& p);



#endif

/// @}
