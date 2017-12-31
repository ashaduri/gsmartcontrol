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
#include <any>


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
		const std::type_info& type = obj->data_.type();
		if (type == typeid(char)) {
			os << std::any_cast<char>(std::any_cast<char>(obj->data_));
		}
		// TODO other types.
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
class ResourceDataAny {

	typedef ResourceDataAny self_type;  ///< Self type

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
			return !data_.has_value();
		}


		/// Clear the data, making it empty
		void clear_data()
		{
			data_.reset();
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
			return data_.type() == typeid(T);
		}


		/// Get data of type \c T.
		/// \return false if casting failed, or if it's empty or invalid type.
		template<typename T>
		inline bool get_data(T& put_it_here) const
		{
			if (const T* value = std::any_cast<T>(&data_)) {
				put_it_here = *value;
				return true;
			}
			return false;
		}


		/// Return a copy of data of type \c T.
		/// \throw rmn::empty_data_retrieval Data is empty
		/// \throw rmn::type_mismatch Type mismatch
		template<typename T>
		T get_data() const
		{
			if (!data_.has_value())
				throw empty_data_retrieval();

			try {
				return std::any_cast<T>(data_);
			}
			catch (std::bad_any_cast& e) {  // convert std::any exception to rmn exception.
				throw type_mismatch(data_.type(), typeid(T));
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

		std::any data_;  ///< The data

};






} // namespace







#endif

/// @}
