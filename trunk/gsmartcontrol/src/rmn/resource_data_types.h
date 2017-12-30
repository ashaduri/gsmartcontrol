/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>

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
/// \file
/// \author Alexander Shaduri
/// \ingroup rmn
/// \weakgroup rmn
/// @{

#ifndef RMN_RESOURCE_DATA_TYPES_H
#define RMN_RESOURCE_DATA_TYPES_H

#include <string>
#include <cstdint>




namespace rmn {



/// Node data type.
/// Only serializable and some additional types are here.
enum node_data_type {
	T_EMPTY,  ///< Not really a type, but may be handy
	T_BOOL,  ///< bool

	T_INT32,  ///< int32_t. NOTE: when writing constants, either \c T_INT32 or \c T_INT64 will be the default.
	T_UINT32,  ///< uint32_t
	T_INT64,  ///< int64_t
	T_UINT64,  ///< uint64_t

	T_DOUBLE,  ///< double. Default when writing floating point constants.
	T_FLOAT,  ///< float.
	T_LDOUBLE,  ///< long double. Not recommended if using windows-based compilers.

	T_STRING,  ///< std::string
	T_VOIDPTR,  ///< void*
	T_UNKNOWN  ///< Unknown type
};



/// Get node_data_type by type \c T. This struct is specialized for different types.
template <typename T>
struct node_data_type_by_real {
	/// Node data type
	static const node_data_type type = T_UNKNOWN;
};

// specializations
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



/// Get node_data_type from rmn node.
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




}  // ns


#endif

/// @}
