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

#ifndef RMN_RESOURCE_DATA_ONE_H
#define RMN_RESOURCE_DATA_ONE_H

#include <string>
#include <iosfwd>  // std::ostream

#include "hz/type_properties.h"  // type_is_same<>
#include "hz/exceptions.h"  // THROW_FATAL
#include "hz/static_assert.h"  // HZ_STATIC_ASSERT

#include "resource_data_types.h"
#include "resource_exception.h"


// a fixed-type data for resource_node.

// Internal Note: Don't use static type checks here - they won't allow
// compiling invalid unused code (like in serializer).


namespace rmn {




// helper class for <<.
template<class T>
struct ResourceDataOneDumper {
	ResourceDataOneDumper(T* o) : obj(o) { }

	void dump(std::ostream& os) const
	{
		if (!obj->data_is_empty())
			os << obj->data_;
	}

	T* obj;
};



template <class T>
inline std::ostream& operator<<(std::ostream& os, const ResourceDataOneDumper<T>& dumper)
{
	dumper.dump(os);
	return os;
}





template<typename DataType, class LockingPolicy>
class ResourceDataOne {

	typedef ResourceDataOne<DataType, LockingPolicy> self_type;
	typedef DataType value_type;  // internal


	public:

		ResourceDataOne() : empty_(true)
		{ }


		template<class T>
		bool copy_data_from(const T& src)  // src node
		{
			return set_data(src->template get_data<DataType>());
		}


		bool data_is_empty() const
		{
			return empty_;
		}


		void clear_data()
		{
			empty_ = true;
		}



		template<typename T>
		inline bool set_data(T data)
		{
			return false;  // invalid type
		}


		inline bool set_data(DataType data)
		{
			data_ = data;
			empty_ = false;
			return true;
		}


		template<typename T>
		inline bool data_is_type() const
		{
			return !empty_ && hz::type_is_same<T, DataType>::value;
		}


		inline bool data_is_type(node_data_type type) const
		{
			return !empty_ && type == node_data_type_by_real<DataType>::type;
		}


		inline node_data_type get_type() const
		{
			if (empty_)
				return node_data_type_by_real<void>::type;
			return node_data_type_by_real<DataType>::type;
		}



		inline bool get_data(DataType& put_it_here) const  // returns false if error
		{
			if (empty_)
				return false;
			put_it_here = data_;
			return true;
		}


		template<typename T>
		T get_data() const  // throws empty_data_retrieval if data is empty
		{
			// use static assertion - early compile-time error is better than runtime error.
			HZ_STATIC_ASSERT((hz::type_is_same<T, DataType>::value), rmn_type_mismatch);
			if (empty_)
				THROW_FATAL(empty_data_retrieval());
			return data_;
		}



		// Use static_cast conversion.
		template<typename T>
		inline bool convert_data(T& put_it_here) const  // returns false if cast failed
		{
			put_it_here = static_cast<T>(data_);
			return true;  // static_cast will check it at compile-time.
		}


		template<typename T>
		T convert_data() const  // throws rmn::empty_data_retrieval if empty
		{
			if (empty_)
				THROW_FATAL(empty_data_retrieval());
			return static_cast<T>(data_);
		}



		template<class T>
		friend struct ResourceDataAnyDumper;


		inline ResourceDataAnyDumper<self_type> dump_data_to_stream() const
		{
			return ResourceDataAnyDumper<const self_type>(this);
		}



	private:

		DataType data_;
		bool empty_;

};




} // namespace







#endif
