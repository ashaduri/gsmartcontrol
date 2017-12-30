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

#ifndef RMN_RESOURCE_DATA_ANY_H
#define RMN_RESOURCE_DATA_ANY_H

#include <string>
#include <iosfwd>  // std::ostream

#include "hz/any_type.h"

#include "resource_data_types.h"
#include "resource_exception.h"


/**
\file
Any-type data for resource_node
*/


namespace rmn {



/// Helper class for operator\<\< with std::ostream.
template<class T>
struct ResourceDataAnyDumper {
	ResourceDataAnyDumper(T* o) : obj(o) { }

	/// Dump resource data into \c os
	void dump(std::ostream& os) const
	{
		os << obj->data_.to_stream();
	}

	T* obj;  ///< ResourceDataAny object
};



/// Output the data into \c os
template <class T>
inline std::ostream& operator<<(std::ostream& os, const ResourceDataAnyDumper<T>& dumper)
{
	dumper.dump(os);
	return os;
}




/// Resource data which can hold variables of any type.
template<class LockingPolicy>
class ResourceDataAny {

	typedef ResourceDataAny<LockingPolicy> self_type;  ///< Self type

	public:

		/// Copy data from \c src node
		template<class T>
		bool copy_data_from(const T& src)
		{
			if (!src)
				return false;

			data_ = src->data_;
			return true;
		}


		/// Check whether data is empty
		bool data_is_empty() const
		{
			return data_.empty();
		}


		/// Clear the data, making it empty
		void clear_data()
		{
			data_.clear();
		}


		/// Set data of any type
		template<typename T>
		inline bool set_data(T data)
		{
			data_ = data;
			return true;
		}


		/// const char* -\> std::string specialization
		inline bool set_data(const char* data)
		{
			data_ = std::string(data);
			return true;
		}


		/// Check whether data is of type \c T.
		/// This function is available only if either RTTI or type tracking is enabled.
		template<typename T>
		inline bool data_is_type() const
		{
			return data_.template is_type<T>();
		}


		/// Get data of type \c T.
		/// \return false if casting failed, or if it's empty or invalid type.
		template<typename T>
		inline bool get_data(T& put_it_here) const
		{
			return data_.get(put_it_here);  // returns false if empty or invalid type
		}


		/// Return a copy of data of type \c T.
		/// \throw rmn::empty_data_retrieval Data is empty
		/// \throw rmn::type_mismatch Type mismatch
		template<typename T>
		T get_data() const
		{
			if (data_.empty())
				throw empty_data_retrieval();

			try {
				// template is needed for gcc 3.3
				return data_.template get<T>();  // won't work if empty or invalid type
			}
			catch (hz::bad_any_cast& e) {  // convert any_type exception to rmn exception.
				throw type_mismatch(data_.type(), typeid(T));
			}
		}



		/// Similar to get_data(), but with looser conversion - can convert between
		/// C++ built-in types and std::string. Uses hz::any_convert<>.
		/// \return false if casting failed, or empty or invalid type.
		template<typename T>
		inline bool convert_data(T& put_it_here) const
		{
			return data_.convert(put_it_here);
		}


		/// Similar to get_data(), but with looser conversion - can convert between
		/// C++ built-in types and std::string. Uses hz::any_convert<>.
		/// \throw rmn::empty_data_retrieval Data is empty
		/// \throw rmn::type_mismatch Type mismatch
		template<typename T>
		T convert_data() const
		{
			if (data_.empty())
				throw empty_data_retrieval();

			try {
				// Note: This throws only if RTTI is enabled.
				// template is needed for gcc 3.3
				return data_.template convert<T>();  // won't work if empty or invalid type
			}
			catch (hz::bad_any_cast& e) {
				throw type_convert_error(data_.type(), typeid(T));
			}
		}



		template<class T>
		friend struct ResourceDataAnyDumper;


		/// Return a helper object that can be dumped into ostream.
		inline ResourceDataAnyDumper<const self_type> dump_data_to_stream() const
		{
			return ResourceDataAnyDumper<const self_type>(this);
		}



	private:

		hz::any_type data_;  ///< The data

};






} // namespace







#endif

/// @}
