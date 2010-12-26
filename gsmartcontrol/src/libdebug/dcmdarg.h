/**************************************************************************
 Copyright:
      (C) 2008 - 2009 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef LIBDEBUG_DCMDARG_H
#define LIBDEBUG_DCMDARG_H

#include "hz/hz_config.h"  // ENABLE_GLIB


// Glib option parsing support - public interface

#ifdef ENABLE_GLIB

#include <glib.h>


// You must not delete the returned value if adding it to context
// (which is usually the case).
// This function will also automatically apply the passed (and default)
// options to all domains.
GOptionGroup* debug_get_option_group();

// Return a string-dump of all debug options (use to debug
// command-line option issues).
std::string debug_get_cmd_args_dump();



#endif  // glib






#endif
