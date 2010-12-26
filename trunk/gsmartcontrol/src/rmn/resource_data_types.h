/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>

 License:

 This software is provided 'as-is', without any express or implied
 warranty.  In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.
***************************************************************************/

#ifndef RMN_RESOURCE_DATA_TYPES_H
#define RMN_RESOURCE_DATA_TYPES_H

#include <string>

#include "hz/hz_config.h"  // DISABLE_RTTI, RMN_TYPE_TRACKING (global_macros.h)
#include "hz/cstdint.h"




namespace rmn {



// only serializable and some additional types here.

enum node_data_type {
	T_EMPTY,  // not really a type, but may be handy
	T_BOOL,

	T_INT32,  // NOTE: when writing constants, either INT32 or INT64 will be the default.
	T_UINT32,
	T_INT64,
	T_UINT64,

	T_DOUBLE,  // default when writing floating point constants.
	T_FLOAT,
	T_LDOUBLE,

	T_STRING,
	T_VOIDPTR,
	T_UNKNOWN
};



template <typename T>
struct node_data_type_by_real { static const node_data_type type = T_UNKNOWN; };

template<> struct node_data_type_by_real<void> { static const node_data_type type = T_EMPTY; };
template<> struct node_data_type_by_real<bool> { static const node_data_type type = T_BOOL; };
template<> struct node_data_type_by_real<int32_t> { static const node_data_type type = T_INT32; };
template<> struct node_data_type_by_real<uint32_t> { static const node_data_type type = T_UINT32; };
template<> struct node_data_type_by_real<int64_t> { static const node_data_type type = T_INT64; };
template<> struct node_data_type_by_real<uint64_t> { static const node_data_type type = T_UINT64; };
template<> struct node_data_type_by_real<double> { static const node_data_type type = T_DOUBLE; };
template<> struct node_data_type_by_real<float> { static const node_data_type type = T_FLOAT; };
template<> struct node_data_type_by_real<long double> { static const node_data_type type = T_LDOUBLE; };
template<> struct node_data_type_by_real<std::string> { static const node_data_type type = T_STRING; };
template<> struct node_data_type_by_real<void*> { static const node_data_type type = T_VOIDPTR; };





#if defined RMN_TYPE_TRACKING && RMN_TYPE_TRACKING

template<class Data> inline
node_data_type resource_node_get_type(intrusive_ptr<const resource_node<Data> > node)
{
	return node->get_type();
}


template<class Data> inline
node_data_type resource_node_get_type(intrusive_ptr<resource_node<Data> > node)
{
	return resource_node_get_type(intrusive_ptr<const resource_node<Data> >(node));
}



#elif !(defined DISABLE_RTTI && DISABLE_RTTI)

// RTTI version (slower)
template<class Data> inline
node_data_type resource_node_get_type(intrusive_ptr<const resource_node<Data> > node)
{
	if (node->data_is_empty()) return node_data_type_by_real<void>::type;
	if (node->template data_is_type<bool>()) return node_data_type_by_real<bool>::type;
	if (node->template data_is_type<int32_t>()) return node_data_type_by_real<int32_t>::type;
	if (node->template data_is_type<uint32_t>()) return node_data_type_by_real<uint32_t>::type;
	if (node->template data_is_type<int64_t>()) return node_data_type_by_real<int64_t>::type;
	if (node->template data_is_type<uint64_t>()) return node_data_type_by_real<uint64_t>::type;
	if (node->template data_is_type<double>()) return node_data_type_by_real<double>::type;
	if (node->template data_is_type<float>()) return node_data_type_by_real<float>::type;
	if (node->template data_is_type<long double>()) return node_data_type_by_real<long double>::type;
	if (node->template data_is_type<std::string>()) return node_data_type_by_real<std::string>::type;
	if (node->template data_is_type<void*>()) return node_data_type_by_real<void*>::type;

	return T_UNKNOWN;
}

template<class Data> inline
node_data_type resource_node_get_type(intrusive_ptr<resource_node<Data> > node)
{
	return resource_node_get_type(intrusive_ptr<const resource_node<Data> >(node));
}

#endif







}  // ns


#endif
