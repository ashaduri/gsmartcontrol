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

#ifndef LIBDEBUG_DEXCEPT_H
#define LIBDEBUG_DEXCEPT_H

#include <stdexcept>  // std::runtime_error



/// Exception thrown on internal libdebug errors
class debug_internal_error : public std::runtime_error {
	public:
		using runtime_error::runtime_error;
};




/// Exception thrown on libdebug API usage errors
class debug_usage_error : virtual public std::runtime_error {
	public:
		using runtime_error::runtime_error;
};






#endif

/// @}
