/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_SYNC_PART_GET_POLICY_H
#define HZ_SYNC_PART_GET_POLICY_H

#include "hz_config.h"  // feature macros


/**
\file
Internal header, do not include manually.
*/

namespace hz {


	/// Get policy class by Mutex type.
	/// Policies should specialize this struct.
	template<class MutexType>
	struct SyncGetPolicy;  // no definition, only specializations.



}  // ns





#endif

/// @}
