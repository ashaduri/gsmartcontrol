/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_PTR_CONTAINER_H
#define HZ_PTR_CONTAINER_H

#include "hz_config.h"  // feature macros

#include "type_properties.h"

/*
A pointer-container wrapper class which auto-deletes its elements.

Heavily based on ptr_container class by Yonat Sharon <yonat 'at' ootips.org>,
from http://ootips.org/yonat/4dev/ .
*/

/*
// Example usage:
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
// v's destructor is called, and it frees the memory allocated for
// the int 17.

Notes:
1. Assumes that all elements are unique (you don't have two elements
	pointing to the same object, otherwise you might delete it twice).
2. Not usable with pair associative containers (map and multimap).

*/


namespace hz {



template<typename Container>
class ptr_container : public Container {
	public:

		// parent's stuff
		typedef typename Container::size_type size_type;
		typedef typename Container::difference_type difference_type;
		typedef typename Container::reference reference;
		typedef typename Container::const_reference const_reference;
		typedef typename Container::value_type value_type;
		typedef typename Container::iterator iterator;
		typedef typename Container::const_iterator const_iterator;
		typedef typename Container::reverse_iterator reverse_iterator;
		typedef typename Container::const_reverse_iterator const_reverse_iterator;

		typedef ptr_container<Container> self_type;
		typedef Container wrapped_type;

		using Container::begin;
		using Container::end;
		using Container::size;


		ptr_container()
		{ }

		ptr_container(const Container& c) : Container(c)
		{ }

		~ptr_container()
		{
			clean_all();
		}

		self_type& operator= (const Container& c)
		{
			Container::operator=(c);
			return *this;
		}


	private:

		// disallow copying of this container, because it the pointers will be copied
		// to and then double-deleted.

		ptr_container(const self_type&);  // private copy constructor

		self_type& operator= (const self_type&);  // private assignment operator



	public:

		void clear()
		{
			clean_all();  // delete pointers
			Container::clear();  // clear the container
		}

		iterator erase(iterator i)
		{
			clean(i);
			return Container::erase(i);
		}

		iterator erase(iterator f, iterator l)
		{
			clean(f,l);
			return Container::erase(f,l);
		}

		// for associative containers: erase() a value
		size_type erase(const value_type& v)
		{
			iterator i = find(v);
			size_type found(i != end()); // can't have more than 1
			if (found)
				erase(i);
			return found;
		}

		// for sequence containers: pop_front(), pop_back(), resize() and assign()
		void pop_front()
		{
			clean(begin());
			Container::pop_front();
		}

		void pop_back()
		{
			iterator i(end());
			clean(--i);
			Container::pop_back();
		}

		void resize(size_type s, value_type c = value_type())
		{
			if (s < size())
				clean(begin()+s, end());
			Container::resize(s, c);
		}

		template <class InputIterator>
		void assign(InputIterator first, InputIterator last)
		{
			clean_all();
			Container::assign(first, last);
		}

		template <class Size, class T>
		void assign(Size n, const T& t = T())
		{
			clean_all();
			Container::assign(n, t);
		}

		// for std::list: remove() and remove_if()
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


		// You MUST delete the elements of put_here.
		template<class ReturnedContainer>
		void clone_to(ReturnedContainer& put_here) const  // deep-clone
		{
			put_here.assign(this->begin(), this->end());
			for (typename ReturnedContainer::iterator i = put_here.begin(); i != put_here.end(); ++i) {
				*i = new typename hz::type_remove_pointer<value_type>::type(*i);
			}
		}

		// You MUST delete the elements of put_here.
		// Same as above, but calls the clone() method of object-to clone.
		// This is needed if we hold the base pointers of child classes.
		template<class ReturnedContainer>
		void clone_by_method_to(ReturnedContainer& put_here) const  // deep-clone
		{
			put_here.assign(this->begin(), this->end());
			for (typename ReturnedContainer::iterator i = put_here.begin(); i != put_here.end(); ++i) {
				*i = (*i)->clone();
			}
		}



	private:

		void clean(iterator i)
		{
			delete *i;
		}

		void clean(iterator first, iterator last)
		{
			while (first != last)
				clean(first++);
		}

		void clean_all()
		{
			clean(begin(), end());
		}

};




}  // ns



#endif
