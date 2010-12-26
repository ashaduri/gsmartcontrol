/**************************************************************************
 Copyright:
      (C) 1999 - 2009  Beman Dawes
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_boost_1_0.txt file
***************************************************************************/

#ifndef HZ_NONCOPYABLE_H
#define HZ_NONCOPYABLE_H

#include "hz_config.h"  // feature macros

// Original notes and copyright info follow:


//  Boost noncopyable.hpp header file  --------------------------------------//

//  (C) Copyright Beman Dawes 1999-2003. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/utility for documentation.



namespace hz {


	//  Private copy constructor and copy assignment ensure classes derived from
	//  class noncopyable cannot be copied.

	//  Contributed by Dave Abrahams

	// namespace is for protection from unintended ADL
	// (Argument-dependent lookup; this avoids unintended lookups in hz namespace).
	namespace noncopyable_ {

		class noncopyable {
			protected:
				noncopyable() {}
				~noncopyable() {}

			private:  // emphasize the following members are private
				noncopyable( const noncopyable& );
				const noncopyable& operator=( const noncopyable& );
		};

	}


	typedef noncopyable_::noncopyable noncopyable;



}  // ns



#endif

