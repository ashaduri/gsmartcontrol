/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
License: Zlib
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup libdebug
/// \weakgroup libdebug
/// @{

#ifndef LIBDEBUG_DCMDARG_H
#define LIBDEBUG_DCMDARG_H

/**
\file
Glib option parsing support - public interface
*/

#if defined ENABLE_GLIB && ENABLE_GLIB

#include <glib.h>


/// Get Glib option group to use for handling libdebug command-line
/// arguments using Glib. You must not delete the returned value if
/// adding it to context (which is usually the case).
/// This function will also automatically apply the passed (and default)
/// options to all domains.
GOptionGroup* debug_get_option_group();

/// Return a string-dump of all libdebug options (use to debug
/// command-line option issues).
std::string debug_get_cmd_args_dump();



#endif  // glib






#endif

/// @}
