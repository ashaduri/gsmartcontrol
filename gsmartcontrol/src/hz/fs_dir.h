/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_FS_DIR_H
#define HZ_FS_DIR_H

#include "hz_config.h"  // feature macros

#include <string>
#include <vector>
#include <sys/types.h>  // dirent needs this

#include "local_algo.h"  // shell_sort
#include "string_wcmatch.h"  // string_wcmatch

#include "fs_path.h"  // Path
#include "fs_dir_platform.h"  // directory_*


// Filesystem directory access.

// This API accepts/gives utf-8 filenames/paths on win32,
// current locale filenames/paths on others (just like glib).



namespace hz {



// -------------------------------------- Iterators


class Dir;

namespace internal {


	// Notes:
	// There is no const version of this because of technical limitations.
	// All iterators to one Dir point to the same entry.

	// Note: Don't forget that there are "." and ".." entries.
	// Use is_special() to verify.

	class DirIterator {
		public:

			typedef void iterator_category;
			typedef std::string value_type;
			typedef int difference_type;
			typedef void pointer;
			typedef value_type reference;

			typedef directory_entry_handle_type handle_type;


			DirIterator() : dir_(NULL), entry_(NULL)
			{ }

			DirIterator(Dir* dir, handle_type entry) : dir_(dir), entry_(entry)
			{ }

			DirIterator(const DirIterator& other)
			{
				dir_ = other.dir_;
				entry_ = other.entry_;
			}

			DirIterator& operator= (const DirIterator& other)
			{
				dir_ = other.dir_;
				entry_ = other.entry_;
				return *this;
			}


			bool operator== (const DirIterator& other) const
			{
				return entry_ == other.entry_;
			}

			bool operator!= (const DirIterator& other) const
			{
				return !(*this == other);
			}


			// Advance to next entry
			inline DirIterator& operator++ ();  // prefix

			// Advance to next entry
			inline void operator++ (int);  // postfix. returns void because of technical limitations.

			// Same as name()
			inline std::string operator* () const;  // dereference

			// Provides operations on return value of name()
			inline std::string operator-> () const;


			// Get entry name
			inline std::string name() const;

			// Get full path
			inline std::string path() const;

			// Returns true if it's "." or "..".
			inline bool is_special() const;

			// Get native handle
			inline handle_type get_handle();


		private:

			Dir* dir_;
			handle_type entry_;

	};


}  // ns




// -------------------------------------- Sorting and filtering functors


// ----------------- Filtering


// No filtering - leave all entries.

struct DirFilterNone {

	// whether to use operator() with FsPaths instead of strings.
	bool use_path_objects()
	{
		return false;
	}

	// no filtering
	bool operator() (const std::string& entry_name)
	{
		return true;
	}

	// dummy
	bool operator() (FsPath& path)
	{
		return true;
	}
};



// Flag-based filtering

enum {  // flags
	// these may be ORed
	DIR_LEAVE_FILE = 1 << 0,
	DIR_LEAVE_DIR = 1 << 1,
	DIR_LEAVE_REGULAR = 1 << 2,
	DIR_LEAVE_SYMLINK = 1 << 3,
	DIR_LEAVE_ALL = DIR_LEAVE_FILE | DIR_LEAVE_DIR  // any entry is either file or directory
};


// Note: If an error occurs while checking flags, the entry is filtered out.
struct DirFilterByFlags {

	DirFilterByFlags(int flags) : flags_(flags)
	{ }

	// whether to use operator() with FsPaths instead of strings.
	bool use_path_objects()
	{
		return true;
	}

	// dummy
	bool operator() (const std::string& entry_name)
	{
		return false;
	}

	bool operator() (FsPath& path)
	{
		if ((flags_ & DIR_LEAVE_FILE) && path.is_file())
			return true;
		if ((flags_ & DIR_LEAVE_DIR) && path.is_dir())
			return true;
		if ((flags_ & DIR_LEAVE_REGULAR) && path.is_regular())
			return true;
		if ((flags_ & DIR_LEAVE_SYMLINK) && path.is_symlink())
			return true;

		return false;
	}

	private:
		int flags_;
};



// glob (aka wildcard: ?, *, []) filtering.

struct DirFilterWc {

	DirFilterWc(const std::string& pattern) : pattern_(pattern)
	{ }

	// whether to use operator() with FsPaths instead of strings.
	bool use_path_objects()
	{
		return false;
	}

	bool operator() (const std::string& entry_name)
	{
		return hz::string_wcmatch(pattern_, entry_name);
	}

	// dummy
	bool operator() (FsPath& path)
	{
		return true;
	}

	private:
		std::string pattern_;
};




// ----------------- Sorting


enum {  // flags
	DIR_SORT_DIRS_FIRST,
	DIR_SORT_FILES_FIRST,
	DIR_SORT_MIXED
};


// No sorting
struct DirSortNone {

	// this is called before the less function
	void set_dir(const std::string& dir)
	{ }

	// whether to use operator() with FsPaths instead of strings.
	bool use_path_objects()
	{
		return false;
	}

	// "less" function using entry names
	bool operator() (const std::string& entry_name1, const std::string& entry_name2)
	{
		return true;  // always return "less", which should cause no sorting.
	}

	// "less" function using path objects
	bool operator() (FsPath& path1, FsPath& path2)
	{
		return true;  // always return "less", which should cause no sorting.
	}
};



// Base class for various sorters
template<class Child>
struct DirSortBase {

	DirSortBase(int flag) : flag_(flag)
	{ }

	// this is called before the less function
	void set_dir(const std::string& dir)
	{
		dir_ = dir;
	}


	// "less" function
	// for  DIR_SORT_MIXED only!
	bool operator() (const std::string& entry_name1, const std::string& entry_name2)
	{
		return static_cast<Child*>(this)->compare(entry_name1, entry_name2);
	}


	// "less" function using path objects
	bool operator() (FsPath& path1, FsPath& path2)
	{
		bool e1_dir = path1.is_dir(), e2_dir = path2.is_dir();

		if (e1_dir == e2_dir)  // same type
			return static_cast<Child*>(this)->compare(path1, path2);

		if (flag_ == DIR_SORT_DIRS_FIRST)
			return e1_dir;  // (e1 < e2) if e1 is dir

		if (flag_ == DIR_SORT_FILES_FIRST)
			return !e1_dir;  // (e1 < e2) if e1 is file

		return true;  // won't reach this
	}


	protected:
		int flag_;
		std::string dir_;
};



// Alphanumeric sorting
struct DirSortAlpha : public DirSortBase<DirSortAlpha> {

	DirSortAlpha(int flag = DIR_SORT_DIRS_FIRST) : DirSortBase<DirSortAlpha>(flag)
	{ }

	// whether to use operator() with FsPaths instead of strings.
	bool use_path_objects()
	{
		// for mixed comparison we don't need to construct the FsPaths.
		return (flag_ != DIR_SORT_MIXED);
	}

	// "less" function (called by parent)
	bool compare(const std::string& entry_name1, const std::string& entry_name2)
	{
		return entry_name1 < entry_name2;
	}

	// "less" function (called by parent)
	bool compare(FsPath& path1, FsPath& path2)
	{
		return path1.str() < path2.str();
	}
};


// Timestamp (descending order) sorting
struct DirSortMTime : public DirSortBase<DirSortMTime> {

	DirSortMTime(int flag = DIR_SORT_DIRS_FIRST) : DirSortBase<DirSortMTime>(flag)
	{ }

	// whether to use operator() with FsPaths instead of strings.
	bool use_path_objects()
	{
		return true;
	}

	// dummy
	bool compare(const std::string& entry_name1, const std::string& entry_name2)
	{
		return true;
	}

	// "less" function (called by parent)
	bool compare(FsPath& path1, FsPath& path2)
	{
		time_t e1_ts = 0, e2_ts = 0;
		if (!path1.get_last_modified(e1_ts) || !path2.get_last_modified(e2_ts) || (e1_ts == e2_ts))
			path1.str() < path2.str();  // error or similar timestamps, fall back to this.

		return e1_ts < e2_ts;
	}

};





// -------------------------------------- Main Dir class


// This class represents a directory which is opened in
// constructor and closed in destructor.
class Dir : public FsPath {
	public:

		typedef internal::DirIterator iterator;

		typedef directory_handle_type handle_type;
		typedef directory_entry_handle_type entry_handle_type;


		// Create a Dir object. This will NOT open the directory.
		Dir() : dir_(NULL), entry_(NULL)
		{ }

		// Create a Dir object. This will NOT open the directory.
		Dir(const FsPath& path) : dir_(NULL), entry_(NULL)
		{
			set_path(path.get_path());
		}

		// Create a Dir object. This will NOT open the directory.
		Dir(const std::string& path) : dir_(NULL), entry_(NULL)
		{
			set_path(path);
		}


	private:
		// Between move semantics (confusing and error-prone) and denying copying,
		// I choose to deny.

		Dir(const Dir& other);  // copy constructor. needed to override Dir(const FsPath&).

		Dir& operator= (const Dir& other);


	public:

/*
		// Copy constructor. Move semantics are implemented -
		// the handle ownership is transferred exclusively.
		Dir(Dir& other) : dir_(NULL), entry_(NULL)
		{
			*this = other;  // operator=
		}

		// Move semantics, as with copy constructor.
		inline Dir& operator= (Dir& other);
*/

		// Destructor which invokes close() if needed.
		virtual ~Dir()
		{
			if (dir_)
				close();
		}


		// Open the directory. The path must be already set.
		inline bool open();

		// Open the directory with path "path".
		inline bool open(const std::string& path);

		// Close the directory manually (automatically invoked in destructor).
		inline bool close();

		// Get native handle of a directory
		inline handle_type get_handle();


		// ------------ directory entry functions

		// Read the next entry. Returns false when the end is reached
		// or if the directory is not open. If entry read error occurs, _true_
		// is returned - to check for error, see bad().
		// This function will open the directory if needed.
		inline bool entry_next();

		// Rewind the entry pointer to the beginning, so that entry_next()
		// will read the first entry.
		// If the directory was changed while open, this should re-read it.
		// This function will open the directory if needed.
		inline bool entry_reset();

		// Get the name of current entry
		inline std::string entry_get_name();

		// Get full path of current entry
		inline std::string entry_get_path();

		// Get native handle of a directory entry
		inline entry_handle_type entry_get_handle();


		// ------------- entry iterator functions

		// Returns an iterator pointing to the first entry.
		// This function will open the directory if needed.
		iterator begin()  // this resets the position!
		{
			entry_reset();
			entry_next();  // set to first entry
			return iterator(this, entry_);
		}

		// Returns an iterator pointing to the entry one past the last one.
		iterator end()
		{
			return iterator(this, NULL);
		}


		// -------------- entry listing

		// Put directory entries into container. Each entry will be filtered through
		// filter_func. The final list will be sorted using sort_func.
		// Container must be a random-access container supporting push_back() method.
		// put_with_path indicates whether the Dir's path should be prepended to
		// entry name before putting it into the container.
		// This function will open the directory if needed.
		template<class Container, class SortFunctor, class FilterFunctor> inline
		bool list(Container& put_here, bool put_with_path, SortFunctor sort_func, FilterFunctor filter_func);

		// Same as above, but defaulting to no filtering.
		template<class Container, class SortFunctor>
		bool list(Container& put_here, bool put_with_path, SortFunctor sort_func)
		{
			return this->list(put_here, put_with_path, sort_func, DirFilterNone());
		}

		// Same as above, but with default sorting.
		template<class Container, class FilterFunctor>
		bool list_filtered(Container& put_here, bool put_with_path, FilterFunctor filter_func)
		{
			return this->list(put_here, put_with_path, DirSortAlpha(DIR_SORT_DIRS_FIRST), filter_func);
		}

		// Same as above, but defaulting to no filtering and alphanumeric sort (dirs first).
		template<class Container>
		bool list(Container& put_here, bool put_with_path = false)
		{
			return this->list(put_here, put_with_path, DirSortAlpha(DIR_SORT_DIRS_FIRST), DirFilterNone());
		}


	private:

		// for move semantics:

// 		Dir(const Dir& other);  // don't allow it. allow only from non-const.

// 		const Dir& operator=(const Dir& other);  // don't allow it. allow only from non-const.


		handle_type dir_;  // DIR handle
		entry_handle_type entry_;  // current entry, dirent*

};





// ------------------------------------------- Implementation


namespace internal {


	inline DirIterator& DirIterator::operator++ ()  // prefix
	{
		if (dir_) {
			dir_->entry_next();
			entry_ = dir_->entry_get_handle();
		} else {
			entry_ = NULL;
		}
		return *this;
	}

	inline void DirIterator::operator++ (int)  // postfix. returns void because of technical limitations.
	{
		++(*this);
	}

	inline std::string DirIterator::operator* () const  // dereference
	{
		return name();
	}

	inline std::string DirIterator::operator-> () const
	{
		return name();
	}


	inline std::string DirIterator::name() const
	{
		if (!entry_ || !dir_)
			return std::string();
		return dir_->entry_get_name();
	}

	inline std::string DirIterator::path() const
	{
		if (!entry_ || !dir_)
			return std::string();
		return dir_->entry_get_path();
	}

	inline bool DirIterator::is_special() const
	{
		std::string n = name();
		return (n == "." || n == "..");
	}

	inline DirIterator::handle_type DirIterator::get_handle()
	{
		if (!entry_ || !dir_)
			return NULL;
		return dir_->entry_get_handle();
	}


}  // ns



// --------------------------------------------

/*
// move semantics, as with copy constructor.
inline Dir& Dir::operator= (Dir& other)
{
	dir_ = other.dir_;
	entry_ = other.entry_;

	// clear other's members, so everything is clear.
	other.dir_ = NULL;
	other.entry_ = NULL;
	other.set_path("");
	other.set_error(HZ__("The directory handle ownership was transferred from this object."));

	return *this;
}
*/


inline bool Dir::open()
{
	if (dir_) {  // already open
		set_error(std::string(HZ__("Error while opening directory \"/path1/\": "))
				+ HZ__("Another directory is open already. Close it first."), 0, this->get_path());
		return false;
	}

	if (this->empty()) {
		set_error(std::string(HZ__("Error while opening directory: ")) + HZ__("Supplied path is empty."));
		return false;
	}

	clear_error();

	dir_ = directory_open(this->c_str());
	if (!dir_) {
		set_error(HZ__("Error while opening directory \"/path1/\": /errno/."), errno, this->get_path());
		return false;
	}

	entry_ = NULL;

	return true;
}



inline bool Dir::open(const std::string& path)
{
	set_path(path);
	return open();
}



inline bool Dir::close()
{
	clear_error();

	if (dir_ && directory_close(dir_) != 0) {  // error
		set_error(HZ__("Error while closing directory \"/path1/\": /errno/."), errno, this->get_path());
		dir_ = NULL;
		entry_ = NULL;
		return false;
	}
	dir_ = NULL;
	entry_ = NULL;

	return true;  // even if already closed
}


inline Dir::handle_type Dir::get_handle()
{
	return dir_;
}



// -------------- directory entry functions

inline bool Dir::entry_next()
{
	clear_error();
	entry_ = NULL;  // just in case

	if (!dir_) {  // open if needed
		if (!open())
			return false;
	}

// 	if (!dir_) {
// 		set_error(HZ__("Error while reading directory entry: Directory is not open.");
// 		return false;  // an indicator to stop the caller's loop.
// 	}

	errno = 0;  // reset errno, because readdir may return NULL even if it's OK.
	entry_ = directory_read(dir_);

	if (errno != 0) {
		set_error(HZ__("Error while reading directory entry of \"/path1/\": /errno/."), errno, this->get_path());
		return true;  // you may continue reading anyway
	}

	if (entry_)
		return true;
	return false;  // end is reached
}


// Rewind the entry pointer to the beginning, so that entry_next() will read the first entry.
// If the directory was changed while open, this should re-read it.
inline bool Dir::entry_reset()
{
	clear_error();
	entry_ = NULL;  // since it's invalid anyway

	if (!dir_) {  // open if needed
		if (!open())
			return false;
	}

// 	if (!dir_) {
// 		set_error(HZ__("Error while reading directory entry: Directory is not open.");
// 		return false;
// 	}

	directory_rewind(dir_);  // no return value here

	return true;
}


// get the name of current entry
inline std::string Dir::entry_get_name()
{
	clear_error();
	if (!dir_) {
		set_error(std::string(HZ__("Error while reading directory entry of \"/path1/\": "))
				+ HZ__("Directory is not open."), 0, this->get_path());
		return std::string();
	}

	if (!entry_) {
		set_error(std::string(HZ__("Error while reading directory entry of \"/path1/\": "))
				+ HZ__("Entry is not set."), 0, this->get_path());
		return std::string();
	}

	return directory_entry_name(entry_);
}


// get full path of current entry
inline std::string Dir::entry_get_path()
{
	return get_path() + DIR_SEPARATOR_S + entry_get_name();
}


// Get native handle of a directory entry
inline Dir::entry_handle_type Dir::entry_get_handle()
{
	return entry_;
}



template<class Container, class SortFunctor, class FilterFunctor> inline
bool Dir::list(Container& put_here, bool put_with_path, SortFunctor sort_func, FilterFunctor filter_func)
{
	clear_error();

	if (!dir_) {  // open if needed
		if (!open())  // this sets the error if needed
			return false;
	}

	entry_reset();


	bool filter_using_paths = filter_func.use_path_objects();
	bool sort_using_paths = sort_func.use_path_objects();

	typedef std::vector<FsPath> list_path_list_t;
	typedef std::vector<std::string> list_string_list_t;
	list_path_list_t path_results;  // for FsPaths
	list_string_list_t string_results;  // for strings


	while (entry_next()) {
		if (bad())
			continue;

		std::string name = entry_get_name();
		if (bad())
			continue;

		if (filter_using_paths) {
			FsPath p(this->get_path());
			p.append(name);

			if (filter_func(p)) {
				if (sort_using_paths) {
					path_results.push_back(p);
				} else {
					string_results.push_back(name);
				}
			}

		} else {  // string-based filtering
			if (filter_func(name)) {
				if (sort_using_paths) {
					FsPath p(this->get_path());
					p.append(name);
					path_results.push_back(p);
				} else {
					string_results.push_back(name);
				}
			}
		}

	}


	sort_func.set_dir(this->get_path());

	if (sort_using_paths) {
		hz::shell_sort(path_results.begin(), path_results.end(), sort_func);

		for (list_path_list_t::const_iterator iter = path_results.begin(); iter != path_results.end(); ++iter) {
			if (put_with_path) {
				put_here.push_back(iter->str());
			} else {
				put_here.push_back(iter->get_basename());
			}
		}

	} else {
		hz::shell_sort(string_results.begin(), string_results.end(), sort_func);

		for (list_string_list_t::const_iterator iter = string_results.begin(); iter != string_results.end(); ++iter) {
			if (put_with_path) {
				FsPath p(this->get_path());
				p.append(*iter);
				put_here.push_back(p.str());

			} else {
				put_here.push_back(*iter);
			}
		}
	}

	return true;
}






}  // ns hz



#endif
