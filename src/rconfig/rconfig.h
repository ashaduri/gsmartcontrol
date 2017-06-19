/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup rconfig
/// \weakgroup rconfig
/// @{

#ifndef RCONFIG_RCONFIG_H
#define RCONFIG_RCONFIG_H


/**
\file
Full rconfig. You may include the individual headers directly to decrease
the number of dependencies.
*/


/**
\namespace rconfig
Rconfig is rmn-based configuration management system.
It provides default config values, config file save / load, etc...
*/


// Full rconfig:

#include "rcmain.h"  // data / node functions.
#include "rcdump.h"  // dumping functions
#include "rcloadsave.h"  // load/save to file/string
#include "rcautosave.h"  // config autosave with timeout



#endif

/// @}
