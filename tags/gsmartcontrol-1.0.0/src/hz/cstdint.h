/**************************************************************************
 Copyright:
      (C) 2009 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef HZ_CSTDINT_H
#define HZ_CSTDINT_H

#include "hz_config.h"  // feature macros


// Use this file instead of stdint.h for portability.
// It contains all the C99 stdint.h stuff in global namespace.


// C++11 defines these automatically for cstdint, so we do it too for compatibility.
#ifndef __STDC_LIMIT_MACROS
	#define __STDC_LIMIT_MACROS
#endif
#ifndef __STDC_CONSTANT_MACROS
	#define __STDC_CONSTANT_MACROS
#endif


// Assume stdint.h is present on all platforms except msvc
#ifdef _MSC_VER

	#include "cstdint_impl_msvc.h"

#else

	#include <stdint.h>

#endif




#endif

/// @}
