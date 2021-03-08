/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gsc
/// \weakgroup gsc
/// @{

#ifndef LOCAL_GLIBMM_H
#define LOCAL_GLIBMM_H


// Glibmm before 2.50.1 uses throw(...) exception specifications which are invalid in C++17.
// Try to work around that.
#ifdef APP_GLIBMM_USES_THROW
	#if !__has_include(<bits/stdc++.h>)
		#error "This version of GLibmm is incompatible with C++17. Use GLibmm 2.50.1 or later."
	#endif

	// include complete stdlib, to avoid throw() macro errors there.
	#include <bits/stdc++.h>  // libstdc++ only

	#define throw(a)  // glibmm uses dynamic exception specifications, remove them.
		#include <glibmm.h>
	#undef throw

#else
	#include <glibmm.h>
#endif

#include <glibmm/i18n.h>

// Undo the macros declared by libintl.h (included by glibmm/i18n.h),
// they conflict with C++ std:: versions.
#undef fprintf
#undef vfprintf
#undef printf
#undef vprintf
#undef sprintf
#undef vsprintf
#undef snprintf
#undef vsnprintf
#undef asprintf
#undef vasprintf
#undef fwprintf
#undef vfwprintf
#undef wprintf
#undef vfwprintf
#undef swprintf
#undef vswprintf
#undef fwprintf
#undef vfwprintf
#undef wprintf
#undef vwprintf
#undef swprintf
#undef vswprintf
#undef setlocale



#endif

/// @}
