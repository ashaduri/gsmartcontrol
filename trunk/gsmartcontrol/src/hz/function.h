/**************************************************************************
 Copyright:
      (C) 2007 - 2011  Thiago Rosso Adams <thiago.adams 'at' gmail.com>
      (C) 2009 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Thiago Rosso Adams
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_SCOPED_PTR_H
#define HZ_SCOPED_PTR_H

#include "hz_config.h"  // feature macros

// TODO: HZIFY

/**
\file
Heavily based on implementation found at
http://www.thradams.com/codeblog/tr1functional2.htm

Original notes and copyright info follow:

Copyright (C) 2007, Thiago Adams (thiago.adams@gmail.com)
This is the implementation of std::tr1::function proposed in tr1.
See details in: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2005/n1836.pdf

Permission to copy, use, modify, sell and distribute this software
is granted provided this copyright notice appears in all copies.
This software is provided "as is" without express or implied
warranty, and with no claim as to its suitability for any purpose.
*/

#include <functional>
#include <cassert>
#include <typeinfo>
#include <stdexcept>

#include "type_properties.h"


namespace hz {

	namespace detail
	{
		template <bool B, class T = void>
		struct disable_if_c { typedef T type; };

		template <class T>
		struct disable_if_c<true, T> {};

		template <class Cond, class T = void>
		struct disable_if : public disable_if_c<Cond::value, T> {};
	}

	//An exception of type bad_function_call is thrown by function::operator() ([3.7.2.4])
	//when the function wrapper object has no target.
	class bad_function_call : public std::runtime_error
	{
	public:
		bad_function_call() : std::runtime_error("call to empty tr1::function") {}
	};

	template<class T> class reference_wrapper
	{
	public:
		typedef T type;

		explicit reference_wrapper(T& t): t_(&t) {}

		operator T& () const { return *t_; }

		T& get() const { return *t_; }

		T* get_pointer() const { return t_; }

	private:

		T* t_;
	};

	//primary template
	template<class T> struct const_mem_fn_t;

	//primary template
	template<class T> struct mem_fn_t;

	//primary template
	template<class T> struct mem_fn_t_binder;

	//primary template
	template<class T> struct const_mem_fn_t_binder;

	//primary template
	template<class F> class function;

	struct unspecified_null_pointer_type {};

	struct function_base
	{
		void *m_pInvoker;

		function_base() : m_pInvoker(0)
		{
		}

		bool empty() const
		{
			return m_pInvoker == 0;
		}
	};

	// [3.7.2.7] Null pointer comparisons
	template <class Function>
	bool operator==(const function_base& f, unspecified_null_pointer_type *)
	{
		return !f;
	}

	template <class Function>
	bool operator==(unspecified_null_pointer_type * , const function_base& f)
	{
		return !f;
	}

	template <class Function>
	bool operator!=(const function_base& f, unspecified_null_pointer_type * )
	{
		return (bool)f;
	}

	template <class Function>
	bool operator!=(unspecified_null_pointer_type *, const function_base& f)
	{
		return (bool)f;
	}

	// [3.7.2.8] specialized algorithms
	template<class Function> void swap(function<Function>& a, function<Function>& b)
	{
		a.swap(b);
	}

} // ns

#define MACRO_JOIN(a, b)        MACRO_DO_JOIN(a, b)
#define MACRO_DO_JOIN(a, b)     MACRO_DO_JOIN2(a, b)
#define MACRO_DO_JOIN2(a, b)    a##b

#define MACRO_MAKE_PARAMS1_0(t)
#define MACRO_MAKE_PARAMS1_1(t)    t##1
#define MACRO_MAKE_PARAMS1_2(t)    t##1, ##t##2
#define MACRO_MAKE_PARAMS1_3(t)    t##1, ##t##2, ##t##3
#define MACRO_MAKE_PARAMS1_4(t)    t##1, ##t##2, ##t##3, ##t##4
#define MACRO_MAKE_PARAMS1_5(t)    t##1, ##t##2, ##t##3, ##t##4, ##t##5

#define MACRO_MAKE_PARAMS2_0(t1, t2)
#define MACRO_MAKE_PARAMS2_1(t1, t2)   t1##1 t2##1
#define MACRO_MAKE_PARAMS2_2(t1, t2)   t1##1 t2##1, t1##2 t2##2
#define MACRO_MAKE_PARAMS2_3(t1, t2)   t1##1 t2##1, t1##2 t2##2, t1##3 t2##3
#define MACRO_MAKE_PARAMS2_4(t1, t2)   t1##1 t2##1, t1##2 t2##2, t1##3 t2##3, t1##4 t2##4
#define MACRO_MAKE_PARAMS2_5(t1, t2)   t1##1 t2##1, t1##2 t2##2, t1##3 t2##3, t1##4 t2##4, t1##5 t2##5


#define MACRO_MAKE_PARAMS1(n, t)         MACRO_JOIN(MACRO_MAKE_PARAMS1_, n) (t)
#define MACRO_MAKE_PARAMS2(n, t1, t2)    MACRO_JOIN(MACRO_MAKE_PARAMS2_, n) (t1, t2)

#define FUNC_NUM_ARGS 0
#include "functional_imp\function_imp.h"
#undef FUNC_NUM_ARGS

#define FUNC_NUM_ARGS 1
#include "functional_imp\function_imp.h"
#undef FUNC_NUM_ARGS

#define FUNC_NUM_ARGS 2
#include "functional_imp\function_imp.h"
#undef FUNC_NUM_ARGS

#define FUNC_NUM_ARGS 3
#include "functional_imp\function_imp.h"
#undef FUNC_NUM_ARGS

#define FUNC_NUM_ARGS 4
#include "functional_imp\function_imp.h"
#undef FUNC_NUM_ARGS

#define FUNC_NUM_ARGS 5
#include "functional_imp\function_imp.h"
#undef FUNC_NUM_ARGS



#endif  // hg

/// @}
