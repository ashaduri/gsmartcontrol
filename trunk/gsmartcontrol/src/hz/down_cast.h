/**************************************************************************
 Copyright:
      (C) 2003 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_DOWN_CAST_H
#define HZ_DOWN_CAST_H

#include "hz_config.h"  // feature macros

#include "static_assert.h"  // HZ_STATIC_ASSERT
#include "type_properties.h"  // type_is_polymorphic, type_is_pointer, type_remove_pointer




namespace hz {


// Perform a down cast.

// This cast works with polymorphic and non-polymorphic types.
// If RTTI is disabled, it will revert to static_cast; otherwise,
// dynamic_cast is used for polymorphic types and static_cast
// for non-polymorphic types.

// Use with pointers only!



#if defined DISABLE_RTTI && DISABLE_RTTI


template<typename Target, typename Source> inline
Target down_cast(const Source& arg)
{
	// accept pointers only:
	HZ_STATIC_ASSERT(type_is_pointer<Target>::value && type_is_pointer<Source>::value, not_a_pointer);

	return static_cast<Target>(arg);
}


#else



namespace internal {

	template<typename Target, typename Source,
			bool Polymorphic = type_is_polymorphic<typename type_remove_pointer<Target>::type>::value
					&& type_is_polymorphic<typename type_remove_pointer<Source>::type>::value >
	struct down_cast_helper
	{
		static Target func(const Source& arg)
		{
			return dynamic_cast<Target>(arg);
		}
	};


	// non-polymorphic specialization
	template<typename Target, typename Source>
	struct down_cast_helper<Target, Source, false>
	{
		static Target func(const Source& arg)
		{
			return static_cast<Target>(arg);
		}
	};

}  // ns internal



template<typename Target, typename Source> inline
Target down_cast(const Source& arg)
{
	// accept pointers only:
	HZ_STATIC_ASSERT(type_is_pointer<Target>::value && type_is_pointer<Source>::value, not_a_pointer);

	return internal::down_cast_helper<Target, Source>::func(arg);
}


#endif  // DISABLE_RTTI




}  // ns




#endif
