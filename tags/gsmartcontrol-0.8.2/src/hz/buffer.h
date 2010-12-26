/**************************************************************************
 Copyright:
      (C) 2006  Applied Informatics Software Engineering GmbH
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_boost_1_0.txt file
***************************************************************************/

#ifndef HZ_BUFFER_H
#define HZ_BUFFER_H

#include "hz_config.h"  // feature macros

#include <cstddef>  // std::size_t
#include <stdexcept>  // std::out_of_range

// Don't use DBG_ASSERT() here, it's too error-prone to use in this context.
#include <cassert>
#ifndef ASSERT  // assert() is undefined if NDEBUG is defined.
	#define ASSERT(a) assert(a)
#endif

#include "exceptions.h"  // THROW_FATAL


// This library is heavily based on Poco::Buffer class from poco
// library.


namespace hz {



// Buffer - a simple scoped buffer.

// Note: Constructing Buffer is not exception-safe!


template<typename T>
class Buffer {
	public:

		Buffer(std::size_t size) : size_(size), ptr_(new T[size])
		{ }

		~Buffer()
		{
			delete[] ptr_;
			ptr_ = 0;  // exception-safety (needed here?)
		}


		std::size_t size() const
		{
			return size_;
		}


		T* begin()
		{
			return ptr_;
		}

		const T* begin() const
		{
			return ptr_;
		}


		// Do NOT dereference the returned value!
		T* end()
		{
			return ptr_ + size_;
		}

		// Do NOT dereference the returned value!
		const T* end() const
		{
			return ptr_ + size_;
		}


		T& operator[] (std::size_t index)
		{
			ASSERT(index >= 0 && index < size_);
			return ptr_[index];
		}

		const T& operator[] (std::size_t index) const
		{
			ASSERT(index >= 0 && index < size_);
			return ptr_[index];
		}


		T& at(std::size_t index)
		{
			if (!(index >= 0 && index < size_))
				THROW_FATAL(std::out_of_range("Buffer::at(): Out of range."));
			return ptr_[index];
		}

		const T& at(std::size_t index) const
		{
			if (!(index >= 0 && index < size_))
				THROW_FATAL(std::out_of_range("Buffer::at(): Out of range."));
			return ptr_[index];
		}



	private:

		Buffer(const Buffer&);  // not implemented

		Buffer& operator= (const Buffer&);  // not implemented

		std::size_t size_;

		T* ptr_;

};




}  // ns



#endif
