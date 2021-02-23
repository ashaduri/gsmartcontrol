
// TODO Remove this in gtkmm4.

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

