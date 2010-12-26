/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_LOCAL_ALGO_H
#define HZ_LOCAL_ALGO_H

#include "hz_config.h"  // feature macros



namespace hz {



// These standard-like algorithms are here to avoid including huge
// standard headers in a header-only library.



// -------------------------------- Shell Sort


namespace internal {

	// Helpers
	template<class T>
	struct shell_sort_helper {  // works for operators
		typedef typename T::value_type value_type;
	};

	template<class T>
	struct shell_sort_helper<T*> {  // needed for C arrays
		typedef T value_type;
	};

}  // ns internal



// Shell sort is one of the fastest on small datasets (<1000),
// and has a small memory footprint.

// Shell sort. Works for most STL containers.
template<class RandomIter> inline
void shell_sort(RandomIter first, RandomIter last)
{
	const int n = last - first;
	for (int gap = n / 2; 0 < gap; gap /= 2) {
		for (int i = gap; i < n; i++) {
			for (int j = i - gap; 0 <= j; j -= gap) {
				if ( *(first+j+gap) < *(first+j) ) {  // swap them
					typename internal::shell_sort_helper<RandomIter>::value_type temp = *(first+j);
					*(first+j) = *(first+j+gap);
					*(first+j+gap) = temp;
				}
			}
		}
	}
}


// Same as above, but with custom comparator function
template<class RandomIter, class Comparator> inline
void shell_sort(RandomIter first, RandomIter last, Comparator less_than_func)
{
	const int n = last - first;
	for (int gap = n / 2; 0 < gap; gap /= 2) {
		for (int i = gap; i < n; i++) {
			for (int j = i - gap; 0 <= j; j -= gap) {
				if ( less_than_func(*(first+j+gap), *(first+j)) ) {  // swap them
					typename internal::shell_sort_helper<RandomIter>::value_type temp = *(first+j);
					*(first+j) = *(first+j+gap);
					*(first+j+gap) = temp;
				}
			}
		}
	}
}




// -------------------------------- Binary Search



template<typename RandomIter, typename ValueType> inline
RandomIter returning_binary_search(RandomIter begin, RandomIter end, const ValueType& value)
{
	RandomIter initial_end = end;
	RandomIter middle;
	while (begin < end) {
		middle = begin + ((end - begin - 1) / 2);
		if (value < *middle) {
			end = middle;
		} else if (*middle < value) {
			begin = middle + 1;
		} else {
			return middle;
		}
	}
	return initial_end;
}


template<typename RandomIter, typename ValueType, typename LessComparator> inline
RandomIter returning_binary_search(RandomIter begin, RandomIter end,
		const ValueType& value, LessComparator comp)
{
	RandomIter initial_end = end;
	RandomIter middle;
	while (begin < end) {
		middle = begin + ((end - begin - 1) / 2);
		if (comp(value, *middle)) {
			end = middle;
		} else if (comp(*middle, value)) {
			begin = middle + 1;
		} else {
			return middle;
		}
	}
	return initial_end;
}






}  // ns





#endif
