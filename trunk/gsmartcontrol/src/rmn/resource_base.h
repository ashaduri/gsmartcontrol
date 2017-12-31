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

#ifndef RMN_RESOURCE_BASE_H
#define RMN_RESOURCE_BASE_H

#include <string>  // names are strings

#include "hz/intrusive_ptr.h"  // intrusive_ptr_referenced class



namespace rmn {



/// This class provides:
/// 1) reference counting (through its base class).
/// 2) naming objects.
class resource_base : public hz::intrusive_ptr_referenced {
	public:

		/// Name comparator
		struct compare_name;

		/// Constructor
		resource_base()
		{ }

		/// Constructor
		resource_base(const std::string& name) : name_(name)
		{ }

		/// Set resource name
		void set_name(const std::string& name)
		{
			name_ = name;
		}

		/// Get resource name
		std::string get_name() const
		{
			return name_;
		}


	private:

		std::string name_;  ///< Resource name

};




struct resource_base::compare_name {

	/// Constructor
	compare_name(const std::string& n)
	{
		name = n;
	}


	/// Check two names for equality
	template<typename T>
	bool operator() (const T& p) const
	{
		if (p->get_name() == name)
			return true;
		return false;
	}


	std::string name;  ///< Name given in constructor

};




}  // ns



#endif

/// @}
