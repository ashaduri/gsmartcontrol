
// TODO Remove this in gtkmm4.

// Glibmm before 2.50.1 uses throw(...) exception specifications which are invalid in C++17.
// Try to work around that.
#ifdef APP_GLIBMM_USES_THROW
	// include stdlib, to avoid throw() macro errors there.
	#include <typeinfo>
	#include <bitset>
	#include <functional>
	#include <utility>
	#include <new>
	#include <memory>
	#include <limits>
	#include <exception>
	#include <stdexcept>
	#include <string>
	#include <vector>
	#include <deque>
	#include <list>
	#include <set>
	#include <map>
	#include <stack>
	#include <queue>
	#include <algorithm>
	#include <iterator>
	#include <ios>
	#include <iostream>
	#include <fstream>
	#include <sstream>
	#include <locale>

	#define throw(a)  // glibmm uses dynamic exception specifications, remove them.
		#include <glibmm.h>
	#undef throw

#else
	#include <glibmm.h>
#endif
