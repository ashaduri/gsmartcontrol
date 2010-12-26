/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>

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

#ifndef RMN_RESOURCE_EXCEPTION_H
#define RMN_RESOURCE_EXCEPTION_H

#include <string>
#include <exception>  // std::exception

#include "hz/bad_cast_exception.h"



namespace rmn {




// This is thrown in case of bad casts.
// Note that this is thrown only if the function has no other means
// of returning an error (e.g. it's returning a reference).

DEFINE_BAD_CAST_EXCEPTION(type_convert_error,
		"Failed data type conversion from \"%s\" to \"%s\".", "Failed data type conversion.");


DEFINE_BAD_CAST_EXCEPTION(type_mismatch,
		"Data type mismatch. Original type: \"%s\", requested type: \"%s\".", "Data type mismatch.");




// This is thrown if attempting to retrieve a data from an empty node.
// Note that this is thrown only if the function has no other means
// of returning an error (e.g. it's returning a reference).

struct empty_data_retrieval : virtual public std::exception {  // from <exception>

	empty_data_retrieval()
	{ }

	// Data injector (which uses this class) doesn't see its path, so no need for this.
// 	explicit empty_data_retrieval(const std::string& from_path) : path(from_path)
// 	{ }

	virtual ~empty_data_retrieval() throw() { }

// 	virtual const char* what() const throw()
// 	{
// 		const char* p = (path.empty() ? "[unknown path]" : path.c_str());
// 		msg = std::string("rmn::empty_data_retrieval: Attempted to retrieve an empty data from \"") + p + "\".";
// 		return msg.c_str();
// 	}

 	virtual const char* what() const throw()
	{
		return "rmn::empty_data_retrieval: Attempt to retrieve data from an empty node.";
	}


// 	std::string path;
// 	mutable std::string msg;  // This must be a member to avoid its destruction on function call return. use what().
};





// This is thrown if attempting to retrieve a data from a non-existent node.
// Note that this is thrown only if the function has no other means
// of returning an error (e.g. it's returning a reference).

struct no_such_node : virtual public std::exception {  // from <exception>

	no_such_node(const std::string& arg_path) : path(arg_path)
	{ }

	virtual ~no_such_node() throw() { }

 	virtual const char* what() const throw()
	{
		msg = "rmn::no_such_node: Attempt to retrieve information about non-existent path: \"" + path + "\".";
		return msg.c_str();
	}

	std::string path;
	mutable std::string msg;  // This must be a member to avoid its destruction on function call return. use what().
};






}  // ns


#endif
