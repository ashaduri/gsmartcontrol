/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>

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

#ifndef RMN_RESOURCE_DATA_ANY_H
#define RMN_RESOURCE_DATA_ANY_H

#include <string>
#include <iosfwd>  // std::ostream

#include "hz/exceptions.h"  // THROW_FATAL
#include "hz/any_type.h"
#include "hz/hz_config.h"  // DISABLE_RTTI, RMN_TYPE_TRACKING (global_macros.h)

#include "resource_data_types.h"
#include "resource_exception.h"


// any-type data for resource_node


namespace rmn {




// helper class for <<.
template<class T>
struct ResourceDataAnyDumper {
	ResourceDataAnyDumper(T* o) : obj(o) { }

	void dump(std::ostream& os) const
	{
		os << obj->data_.to_stream();
	}

	T* obj;
};



template <class T>
inline std::ostream& operator<<(std::ostream& os, const ResourceDataAnyDumper<T>& dumper)
{
	dumper.dump(os);
	return os;
}





template<class LockingPolicy>
class ResourceDataAny {

	typedef ResourceDataAny<LockingPolicy> self_type;


	public:

		ResourceDataAny()
#ifdef RMN_TYPE_TRACKING
			: type_(T_EMPTY)
#endif
		{ }



		template<class T>
		bool copy_data_from(const T& src)  // src node
		{
			if (!src)
				return false;

			data_ = src->data_;
#ifdef RMN_TYPE_TRACKING
			type_ = src->get_type();
#endif
			return true;
		}


		bool data_is_empty() const
		{
			return data_.empty();
		}


		void clear_data()
		{
			data_.clear();
#ifdef RMN_TYPE_TRACKING
			type_ = T_EMPTY;
#endif
		}



		template<typename T>
		inline bool set_data(T data)
		{
			data_ = data;
#ifdef RMN_TYPE_TRACKING
			type_ = node_data_type_by_real<T>::type;
#endif
			return true;
		}


		// const char* -> std::string specialization
		inline bool set_data(const char* data)
		{
			data_ = std::string(data);
#ifdef RMN_TYPE_TRACKING
			type_ = node_data_type_by_real<std::string>::type;
#endif
			return true;
		}


		// this function works only if either RTTI or type tracking is enabled
#ifndef DISABLE_RTTI
		template<typename T>
		inline bool data_is_type() const
		{
			return data_.is_type<T>();  // won't work without RTTI! If empty, reacts to void only.
		}
#elif defined RMN_TYPE_TRACKING
		template<typename T>
		inline bool data_is_type() const
		{
			return node_data_type_by_real<T>::type == type_;
		}
#endif



#ifdef RMN_TYPE_TRACKING
		inline bool data_is_type(node_data_type type) const
		{
			return type == type_;
		}

		inline node_data_type get_type() const
		{
			return type_;
		}
#endif


		template<typename T>
		inline bool get_data(T& put_it_here) const  // returns false if cast failed
		{
#ifdef RMN_TYPE_TRACKING
			if (node_data_type_by_real<T>::type != type_)
				return false;
#endif
			return data_.get(put_it_here);  // returns false if empty or invalid type
		}


		// This function throws if:
		//	* data empty (rmn::empty_data_retrieval);
		//	* type mismatch (rmn::type_mismatch).
		template<typename T>
		T get_data() const
		{
			if (data_.empty())
				THROW_FATAL(empty_data_retrieval());

#if defined DISABLE_RTTI && defined RMN_TYPE_TRACKING
			if (node_data_type_by_real<T>::type != type_)
				THROW_FATAL(type_mismatch());
#endif
			try {
				return data_.get<T>();  // won't work if empty or invalid type
			}
			catch (hz::bad_any_cast& e) {  // convert any_type exception to rmn exception.
				THROW_CUSTOM_BAD_CAST(type_mismatch, data_.type(), typeid(T));
			}
		}



		// More loose conversion - can convert between C++ built-in types and std::string.
		// Uses any_convert<>.
		template<typename T>
		inline bool convert_data(T& put_it_here) const  // returns false if cast failed
		{
			return data_.convert(put_it_here);  // returns false if empty or invalid type
		}


		// This function throws if:
		//	* data empty (rmn::empty_data_retrieval);
		//	* type conversion error (rmn::type_convert_error).
		template<typename T>
		T convert_data() const
		{
			if (data_.empty())
				THROW_FATAL(empty_data_retrieval());

#if defined DISABLE_RTTI && defined RMN_TYPE_TRACKING
			node_data_type to = node_data_type_by_real<T>::type;

			// any T_ type is ok except these:
			if (type_ == T_VOIDPTR || type_ == T_UNKNOWN
					|| to == T_VOIDPTR || to == T_UNKNOWN) {
				THROW_FATAL(type_convert_error());
			}
#endif
			try {
				// Note: This throws only if RTTI is enabled.
				return data_.convert<T>();  // won't work if empty or invalid type
			}
			catch (hz::bad_any_cast& e) {
				THROW_CUSTOM_BAD_CAST(type_convert_error, data_.type(), typeid(T));
			}
		}




		template<class T>
		friend struct ResourceDataAnyDumper;


		inline ResourceDataAnyDumper<const self_type> dump_data_to_stream() const
		{
			return ResourceDataAnyDumper<const self_type>(this);
		}



	private:

		hz::any_type data_;

#ifdef RMN_TYPE_TRACKING
		node_data_type type_;
#endif

};






} // namespace







#endif
