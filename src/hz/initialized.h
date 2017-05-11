/**************************************************************************
 Copyright:
      (C) 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_INITIALIZED_H
#define HZ_INITIALIZED_H

#include "hz_config.h"  // feature macros



namespace hz {


/// This template is useful for default-initializing POD class members.
template<class T>
class initialized {
	public:

		initialized() : x()
		{ }

		initialized(const initialized& arg) : x(arg.x)
		{ }

		explicit initialized(const T& arg) : x(arg)
		{ }

		initialized& operator=(const initialized& arg)
		{
			x = arg.x;
			return *this;
		}

		initialized& operator=(const T& arg)
		{
			x = arg;
			return *this;
		}

		operator const T&() const
		{
			return x;
		}

		operator T&()
		{
			return x;
		}

		const T& data() const
		{
			return x;
		}

		T& data()
		{
			return x;
		}


	protected:

		T x ;

} ;



/// This template specialization is useful for default-initializing raw pointers.
template<class T>
class initialized<T*> {
	public:

		initialized() : x()
		{ }

		initialized(const initialized& arg) : x(arg.x)
		{ }

		explicit initialized(T* arg) : x(arg)
		{ }

		initialized& operator=(const initialized& arg)
		{
			x = arg.x;
			return *this;
		}

		initialized& operator=(T* arg)
		{
			x = arg;
			return *this;
		}

		T* operator->()
		{
			return x;
		}

		operator const T*() const
		{
			return x;
		}

		// Avoid gcc warning about choosing one cast over another (-Wconversion)
// 		operator T*&()
// 		{
// 			return x;
// 		}

		const T*& data() const
		{
			return x;
		}

		T*& data()
		{
			return x;
		}


	protected:

		T* x ;

} ;



/// This template is useful for initializing enum-type and integral variables.
template<class T, T initial_value>
class value_initialized : public initialized<T> {
	public :

		value_initialized() : initialized<T>(initial_value)
		{ }

		value_initialized(const initialized<T>& arg) : initialized<T>(arg)
		{ }

		explicit value_initialized(const T& arg) : initialized<T>(arg)
		{ }

		value_initialized& operator=(const initialized<T>& arg)
		{
			initialized<T>::operator=(arg);
			return *this;
		}

		value_initialized& operator=(const T& arg)
		{
			initialized<T>::operator=(arg);
			return *this;
		}

} ;








}  // ns



#endif

/// @}
