/**************************************************************************
 Copyright:
      (C) 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
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
	namespace fs = std::experimental::filesystem;
#endif


}  // ns hz



#endif

/// @}
