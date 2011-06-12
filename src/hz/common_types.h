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


// A type that may be specified for template arguments,
// to avoid specifying the real type.

/*
// sample usage:
template<typename T = DefT>
struct A {
	// this will select DefT if DefaultType was passed for T.
	typedef typename type_auto_select<T, DefT>::type RealT;
};
*/


struct DefaultType { };  // dummy class


template<typename PassedT, typename DefaultT, typename AutoT = DefaultType>
struct type_auto_select {
	typedef PassedT type;
};

// AutoT was passed, select default.
template<typename PassedT, typename DefaultT>
struct type_auto_select<PassedT, DefaultT, PassedT> {
	typedef DefaultT type;
};




// ---------------------------------- NullType


// NullType is usually used as default template argument
// to indicate that it's unused.

struct NullType
{ };


// In case you need multiple unique NullType-s, use this
// with different N values.

template<int N>
struct NullTypeUnique
{ };




}  // ns



// using hz::DefaultType;  // for convenience.





#endif
