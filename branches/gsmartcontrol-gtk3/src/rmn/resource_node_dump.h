/**************************************************************************
 Copyright:
      (C) 2003 - 2010  Irakli Elizbarashvili <ielizbar 'at' gmail.com>
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
/// \author Irakli Elizbarashvili
/// \author Alexander Shaduri
/// \ingroup rmn
/// \weakgroup rmn
/// @{

#ifndef RMN_RESOURCE_NODE_DUMP_H
#define RMN_RESOURCE_NODE_DUMP_H

#include <string>
#include <sstream>
#include <iosfwd>  // std::ostream
#include <iomanip>  // setw(), left()

#include "resource_node.h"



namespace rmn {


// Unfortunately, these functions cannot accept
// resource_node<Data>::node_const_ptr
// as parameters, because of something called non-deducible
// context (regarding nested type).


namespace internal {

	/// A helper for resource_node_dump_recursive()
	template<class Data> inline
	std::string resource_node_dump_recursive_helper(intrusive_ptr<const resource_node<Data> > node,
			int internal_dump_offset = 0)
	{
		std::string str;

	// 	int len = node->get_name().size();
		int fill = 20 - internal_dump_offset;

		std::ostringstream ss;
		ss << std::string(internal_dump_offset, ' ');
		// "refcount - 1" is needed because we take one reference (argument of this function).
		ss << std::left << std::setw(fill) << node->get_name() << " [" << (node->ref_count() - 1) << "] "
			<< std::setw(20) << node->get_path()
			<< " " << std::setw(10) << node->dump_data_to_stream()
			<< std::setw(0) << "\n";

		str += ss.str();

		typename resource_node<Data>::child_const_iterator iter = node->children_begin();
		for (; iter != node->children_end(); ++iter) {
			str += resource_node_dump_recursive(*iter, internal_dump_offset + 2);
		}

		return str;
	}

}


/// Dump node recursively in pretty ascii format (suitable for reading/debugging only).
template<class Data> inline
std::string resource_node_dump_recursive(intrusive_ptr<const resource_node<Data> > node)
{
	return internal::resource_node_dump_recursive_helper(node);
}



// Why the hell doesn't the one above capture this one too?
/// Non-const overload.
template<class Data> inline
std::string resource_node_dump_recursive(intrusive_ptr<resource_node<Data> > node,
		int internal_dump_offset = 0)
{
	return resource_node_dump_recursive(intrusive_ptr<const resource_node<Data> >(node));
}



/// Dump node children's _data_ only (non-recursively) in ascii format (suitable for reading/debugging only).
template<class Data> inline
std::string resource_node_dump_children_data(intrusive_ptr<const resource_node<Data> > node)
{
	std::string str;

	if (!node->get_child_count())
		return str;

	typename resource_node<Data>::child_const_iterator iter = node->children_begin();
	for (; iter != node->children_end(); ++iter) {
		str += (*iter)->dump_data_to_stream() << "\n";
	}

	return str;
}


/// Non-const overload.
template<class Data> inline
std::string resource_node_dump_children_data(intrusive_ptr<resource_node<Data> > node)
{
	return resource_node_dump_children_data(intrusive_ptr<const resource_node<Data> >(node));
}




} // namespace rmn




/// std::ostream output operator for resource node.
/// This needs to be in global namespace, else it will be looked up in hz::, not rmn::.
template<class Data> inline
std::ostream& operator<< (std::ostream& os, rmn::intrusive_ptr<const rmn::resource_node<Data> > node)
{
	return (os << rmn::resource_node_dump_recursive(node));
}



/// std::ostream output operator for resource node.
/// This needs to be in global namespace, else it will be looked up in hz::, not rmn::.
template<class Data> inline
std::ostream& operator<< (std::ostream& os, rmn::intrusive_ptr<rmn::resource_node<Data> > node)
{
	return (os << rmn::resource_node_dump_recursive(node));
}







#endif

/// @}
