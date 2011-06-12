/**************************************************************************
 Copyright:
      (C) 2003 - 2010  Irakli Elizbarashvili <ielizbar 'at' gmail.com>
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>

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

#ifndef RMN_RMN_H
#define RMN_RMN_H


// Include this file to use the whole rmn.
// Otherwise, include individual headers.


// Configuration macros:

// RMN_TYPE_TRACKING to enable explicit data type tracking in
// resource_data providers.

// Note: Either RTTI or RMN_TYPE_TRACKING must be enabled to
// be able to serialize the node data! (Loading the serialized data will still work).

// Note: Type tracking allows you to track them only from a predefined
// set of types.

// Type tracking and RTTI effects on resource_data_any:

// Type tracking (RMN_TYPE_TRACKING):
// - Fast (one integer comparison when checking types).
// - One more member variable in each node.
// - All type checks are limited to predefined types.

// RTTI (disabled through DISABLE_RTTI, otherwise always present):
// - Possibly slower (type checks are performed through std::type_info comparisons).
// - No additional member variables.
// - Can check any type, thus it's more reliable when working with
// non-predefined types.



// #define RMN_TYPE_TRACKING


#include "resource_base.h"  // needed for node
#include "resource_node.h"  // resource_node type
#include "resource_exception.h"  // exceptions thrown in critical situations
#include "resource_data_types.h"  // type tracking helpers for data
#include "resource_data_locking.h"  // locking policies for data
#include "resource_data_any.h"  // any_type data provider
#include "resource_data_one.h"  // a specific type data provider
#include "resource_node_dump.h"  // node dumper functions
#include "resource_serialization.h"  // serialize / unserialize nodes








#endif

