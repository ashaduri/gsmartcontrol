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

#ifndef RMN_RESOURCE_DATA_ONE_H
#define RMN_RESOURCE_DATA_ONE_H

#include <string>
#include <iosfwd>  // std::ostream
#include <type_traits>

#include "resource_data_types.h"
#include "resource_exception.h"



namespace rmn {


// Internal Note: Don't use static type checks here - they won't allow
// compiling invalid unused code (like in serializer).


/// Helper class for operator\<\< with std::ostream.
template<class T>
struct ResourceDataOneDumper {
	ResourceDataOneDumper(T* o) : obj(o) { }

	/// Dump resource data into \c os
	void dump(std::ostream& os) const
	{
		if (!obj->data_is_empty())
			os << obj->data_;
	}

	T* obj;  ///< ResourceDataOne object
};



/// Output the data into \c os
template <class T>
inline std::ostream& operator<<(std::ostream& os, const ResourceDataOneDumper<T>& dumper)
{
	dumper.dump(os);
	return os;
}



/// Resource data which can handle data of \c DataType only.
template<typename DataType, class LockingPolicy>
class ResourceDataOne {

	typedef ResourceDataOne<DataType, LockingPolicy> self_type;  ///< Self type
	typedef DataType value_type;  ///< DataType

	public:

		/// Constructor
		ResourceDataOne() : empty_(true)
		{ }


		/// Copy data from \c src node
		template<class T>
		bool copy_data_from(const T& src)
		{
			return set_data(src->template get_data<DataType>());
		}


		/// Check whether data is empty
		bool data_is_empty() const
		{
			return empty_;
		}


		/// Clear the data, making it empty
		void clear_data()
		{
			empty_ = true;
		}


		/// Set data of any type. This operation will fail for all types except DataType.
		template<typename T>
		inline bool set_data(T data)
		{
			return false;  // invalid type
		}


		/// Set data of type DataType
		inline bool set_data(DataType data)
		{
			data_ = data;
			empty_ = false;
			return true;
		}


		/// Check whether data is of type \c T.
		template<typename T>
		inline bool data_is_type() const
		{
			return !empty_ && std::is_same_v<T, DataType>;
		}


		/// Check whether data is of type \c type.
		inline bool data_is_type(node_data_type type) const
		{
			return !empty_ && type == node_data_type_by_real<DataType>::type;
		}


		/// Get data type
		inline node_data_type get_type() const
		{
			if (empty_)
				return node_data_type_by_real<void>::type;
			return node_data_type_by_real<DataType>::type;
		}


		/// Get data.
		/// \return false on error.
		inline bool get_data(DataType& put_it_here) const
		{
			if (empty_)
				return false;
			put_it_here = data_;
			return true;
		}


		/// Get a copy of data.
		/// \throw rmn::empty_data_retrieval Data is empty
		template<typename T>
		T get_data() const
		{
			// use static assertion - early compile-time error is better than runtime error.
			static_assert((std::is_same_v<T, DataType>), "rmn type mismatch");
			if (empty_)
				throw empty_data_retrieval();
			return data_;
		}


		/// Get data (use static_cast conversion).
		template<typename T>
		inline bool convert_data(T& put_it_here) const  // returns false if cast failed
		{
			put_it_here = static_cast<T>(data_);
			return true;  // static_cast will check it at compile-time.
		}


		/// Get a copy of data (use static_cast conversion).
		/// \throw rmn::empty_data_retrieval if data is empty
		template<typename T>
		T convert_data() const
		{
			if (empty_)
				throw empty_data_retrieval();
			return static_cast<T>(data_);
		}



		template<class T>
		friend struct ResourceDataOneDumper;


		/// Return a helper object that can be dumped into ostream.
		inline ResourceDataOneDumper<self_type> dump_data_to_stream() const
		{
			return ResourceDataOneDumper<const self_type>(this);
		}



	private:

		DataType data_;  ///< The data
		bool empty_;  ///< Whether the data is empty or not.

};




} // namespace







#endif

/// @}
