/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Yonat Sharon
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_PTR_CONTAINER_H
#define HZ_PTR_CONTAINER_H

#include "hz_config.h"  // feature macros

#include <type_traits>



namespace hz {


/**
A pointer-container wrapper class which auto-deletes its elements.

Heavily based on ptr_container class by Yonat Sharon <yonat 'at' ootips.org>,
from http://ootips.org/yonat/4dev/ .

Example usage:
\code
{
	ptr_container<std::vector<int*> > v;
	// v can be manipulated like any std::vector<int*>.

	v.push_back(new int(42));
	v.push_back(new int(17));
	// v now owns the allocated int-s. Don't free them elsewhere!

	v.erase(v.begin());
	// frees the memory allocated for the int 42, and then removes the
	// first element of v.
}
// v's destructor is called, and it frees the memory allocated for the int 17.
\endcode

Notes:
1. Assumes that all elements are unique (you don't have two elements
	pointing to the same object, otherwise you might delete it twice).
2. Not usable with pair associative containers (map and multimap).
*/
template<typename Container>
class ptr_container : public Container {
	public:

		// parent's stuff
		typedef typename Container::size_type size_type;  ///< STL compatibility
		typedef typename Container::difference_type difference_type;  ///< STL compatibility
		typedef typename Container::reference reference;  ///< STL compatibility
		typedef typename Container::const_reference const_reference;  ///< STL compatibility
		typedef typename Container::value_type value_type;  ///< STL compatibility
		typedef typename Container::iterator iterator;  ///< STL compatibility
		typedef typename Container::const_iterator const_iterator;  ///< STL compatibility
		typedef typename Container::reverse_iterator reverse_iterator;  ///< STL compatibility
		typedef typename Container::const_reverse_iterator const_reverse_iterator;  ///< STL compatibility

		typedef ptr_container<Container> self_type;  ///< Type of this class
		typedef Container wrapped_type;  ///< Wrapped container type

		using Container::begin;
		using Container::end;
		using Container::size;


		/// Constructor
		ptr_container()
		{ }

		/// Construct from existing container
		ptr_container(const Container& c) : Container(c)
		{ }

		/// Destructor, deletes all elements
		~ptr_container()
		{
			clean_all();
		}

		/// Assignment operator (from existing container)
		self_type& operator= (const Container& c)
		{
			Container::operator=(c);
			return *this;
		}


	private:

		/// Private copy constructor (disallows copying and therefore double-deletion)
		ptr_container(const self_type&);

		/// Private assignment operator (disallows copying and therefore double-deletion)
		self_type& operator= (const self_type&);



	public:

		/// Delete all elements, clear the container
		void clear()
		{
			clean_all();  // delete pointers
			Container::clear();  // clear the container
		}

		/// Delete one element, remove it from container
		iterator erase(iterator i)
		{
			clean(i);
			return Container::erase(i);
		}

		/// Delete a range of elements, remove them from container
		iterator erase(iterator f, iterator l)
		{
			clean(f,l);
			return Container::erase(f,l);
		}

		/// For associative containers: erase() a value
		size_type erase(const value_type& v)
		{
			iterator i = Container::find(v);
			size_type found(i != end()); // can't have more than 1
			if (found)
				erase(i);
			return found;
		}

		/// Delete the first element, remove it from container.
		void pop_front()
		{
			clean(begin());
			Container::pop_front();
		}

		/// Delete the last element, remove it from container.
		void pop_back()
		{
			iterator i(end());
			clean(--i);
			Container::pop_back();
		}

		/// Resize the container. If the new size is smaller than the current
		/// one, delete the extra elements and remove them.
		void resize(size_type s, value_type c = value_type())
		{
			if (s < size())
				clean(begin()+s, end());
			Container::resize(s, c);
		}

		/// Clear the container (deleting everything) and assign a new range to it.
		template <class InputIterator>
		void assign(InputIterator first, InputIterator last)
		{
			clean_all();
			Container::assign(first, last);
		}

		/// Clear the container (deleting everything) and create a range of \c n elements.
		template <class Size, class T>
		void assign(Size n, const T& t = T())
		{
			clean_all();
			Container::assign(n, t);
		}

		/// For std::list: remove by value (delete if found)
		void remove(const value_type& v)
		{
			for (iterator i = begin(); i != end(); ++i) {
				if ((*i) == v) {
					clean(i);
					break;
				}
			}
// 			clean( std::find(begin(), end(), v) );
			Container::remove(v);
		}

		/// For std::list: remove by predicate (delete if found)
		template <class Pred>
		void remove_if(Pred pr)
		{
			for (iterator i = begin(); i != end(); ++i) {
				if (pr(*i))
					clean(i);
			}
			Container::remove_if(pr);
		}


		// Additional, non-Container-emulating functions:


		/// Deep-clone the container and its elements.
		/// You MUST delete the elements of \c put_here after you're done with them.
		template<class ReturnedContainer>
		void clone_to(ReturnedContainer& put_here) const
		{
			put_here.assign(this->begin(), this->end());
			for (typename ReturnedContainer::iterator i = put_here.begin(); i != put_here.end(); ++i) {
				*i = new typename std::remove_pointer_t<value_type>(*i);
			}
		}

		/// Same as clone_to(), but calls the clone() method of objects-to clone.
		/// This is needed if we hold the base pointers of child classes.
		/// You MUST delete the elements of put_here.
		template<class ReturnedContainer>
		void clone_by_method_to(ReturnedContainer& put_here) const  // deep-clone
		{
			put_here.assign(this->begin(), this->end());
			for (typename ReturnedContainer::iterator i = put_here.begin(); i != put_here.end(); ++i) {
				*i = (*i)->clone();
			}
		}


	private:

		/// Delete element
		void clean(iterator i)
		{
			delete *i;
		}

		/// Delete range
		void clean(iterator first, iterator last)
		{
			while (first != last)
				clean(first++);
		}

		/// Delete all elements
		void clean_all()
		{
			clean(begin(), end());
		}

};




}  // ns



#endif

/// @}
