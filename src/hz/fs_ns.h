/******************************************************************************
License: Zlib
Copyright:
	(C) 2018 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_FS_NS_H
#define HZ_FS_NS_H

#if __has_include(<filesystem>)
	#include <filesystem>
#else
	#include <experimental/filesystem>
#endif


/**
\file
Filesystem utilities
*/


namespace hz {


#ifdef __cpp_lib_filesystem

	namespace fs = std::filesystem;

#else  // __cpp_lib_experimental_filesystem

	// Note: This requires -lstdc++fs with gcc's libstdc++, -lc++experimental with clang's libc++.
	namespace fs = std::experimental::filesystem;

#endif


}  // ns hz



#endif

/// @}
