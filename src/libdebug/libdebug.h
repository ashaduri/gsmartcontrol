/******************************************************************************
License: Zlib
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup libdebug
/// \weakgroup libdebug
/// @{

#ifndef LIBDEBUG_LIBDEBUG_H
#define LIBDEBUG_LIBDEBUG_H

/**
\file
Main libdebug include file, includes complete libdebug functionality.
\see libdebug_mini.h
*/

// These macros may be used to control how libdebug is built:

/// \def ENABLE_GLIB
/// Define to 1 to enable Glib option parsing support.

/// \def DEBUG_BUILD
/// Define to 1 to enable all levels by default.


/// \namespace debug_internal
/// Libdebug internal implementation details



// all libdebug headers:

#include "dchannel.h"
#include "dcmdarg.h"
#include "dexcept.h"
#include "dflags.h"
#include "dout.h"
#include "dstate_pub.h"
// #include "dstream.h"  // no dstream - it's internal only



#endif

/// @}
