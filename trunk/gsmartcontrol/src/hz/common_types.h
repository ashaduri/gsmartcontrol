/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_COMMON_TYPES_H
#define HZ_COMMON_TYPES_H

#include "hz_config.h"  // feature macros



namespace hz {


// ---------------------------------- DefaultType



/// A type that may be specified for template arguments,
/// to avoid specifying the real type.
struct DefaultType { };  // dummy class


/**
Set \c type to \c DefaultT if \c AutoT hz::DefaultType, otherwise set it to \c PassedT.
\code
	// Sample usage:
	template<typename T>
	struct A {
		// this will select DefT if hz::DefaultType was passed for T.
		typedef typename hz::type_auto_select<T, std::stringstream>::type RealT;
	};

	A<hz::DefaultType>::RealT a1;  ///< a1 is of std::stringstream type.
	A<std::string>::RealT a1;  ///< a1 is of std::string type.
\endcode
*/
template<typename PassedT, typename DefaultT, typename AutoT = DefaultType>
struct type_auto_select {
	typedef PassedT type;  ///< Deduced type
};

/// Specialization, DefaultType was passed, select default.
template<typename PassedT, typename DefaultT>
struct type_auto_select<PassedT, DefaultT, PassedT> {
	typedef DefaultT type;  ///< Deduced type
};




// ---------------------------------- NullType


/// NullType is usually used as default template argument
/// to indicate that it's unused.
struct NullType
{ };


/// In case you need multiple unique NullType-style classes, use this
/// with different N values.
template<int N>
struct NullTypeUnique
{ };




}  // ns



// using hz::DefaultType;  // for convenience.





#endif

/// @}
