/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_FS_PATH_H
#define HZ_FS_PATH_H

#include "hz_config.h"  // feature macros

#include <string>
#include <cerrno>  // errno (not std::errno, it may be a macro)
#include <stdio.h>  // (not cstdio!) FILE, fopen and friends; remove()
#include <sys/types.h>  // stat.h, dirent.h, utime.h need this; time_t; mode_t
#include <sys/stat.h>  // stat() needs this; chmod(); mkdir()
#include <dirent.h>  // opendir() and friends
#include <utime.h>  // utime()

#ifdef _WIN32
	#include <io.h>  // access(), stat(), unlink(), rmdir()
#else
	#include <unistd.h>  // access(), stat(), unlink(), readlink()
#endif

#include "fs_common.h"  // separator
#include "fs_path_utils.h"  // path_* functions
#include "fs_error_holder.h"  // FsErrorHolder


// Filesystem path and file manipulation


namespace hz {




class FsPath : public FsErrorHolder {

	public:

		FsPath()
		{ }

		// you should check the success status with bad().
		FsPath(const std::string& path) : path_(path)
		{ }



		// --- these will _not_ set bad() status

		void set_path(const std::string& path)
		{
			path_ = path;
		}

		std::string get_path() const
		{
			return path_;
		}

		// same as get_path()
		std::string str() const
		{
			return path_;
		}

		// same as get_path().c_str()
		const char* c_str() const
		{
			return path_.c_str();
		}


		// Convert path from unknown format to native (e.g. unix paths to win32).
		// The current object is also modified.
		inline FsPath& to_native();

		// Remove trailing separators in path (unless they are parts of root component).
		// The current object is also modified.
		inline FsPath& trim_trailing();

		// Go up "steps" steps. The current object is also modified.
		inline FsPath& go_up(unsigned int steps = 1);

		// Append a partial (e.g. relative) path. It doesn't matter if it starts with a
		// separator. The current object is also modified.
		inline FsPath& append(const std::string& partial_path);

		// Compress a path - remove double separators, trailing
		// separator, "/./" components, and deal with "/../" if possible.
		// Note: This function performs its operations on strings, not real paths.
		// The current object is also modified.
		inline FsPath& compress();


		// get the path truncated by 1 level, e.g. /usr/local/ -> /usr.
		inline std::string get_dirname() const;

		// get the basename of path, e.g. /usr/local/ -> local; /a/b/c -> c.
		inline std::string get_basename() const;

		// Get root path of current path. e.g. '/' or 'D:\'.
		// May not work with relative paths under win32.
		inline std::string get_root() const;

		// Check if the path corresponds to root (drive / share in win32).
		inline bool is_root() const;

		// Get an extension of the last component.
		inline std::string get_extension() const;

		// check if the path is absolute (only for native paths). returns 0 if it's not.
		// the returned value is a position past the root component (e.g. 3 for C:\temp).
		inline std::string::size_type is_absolute() const;

		// check if the current path is a subpath of supplied argument.
		// e.g. /usr/local/bin is a subpath of /usr. Note: This doesn't check real paths, only strings.
		inline bool is_subpath_of(const std::string& superpath) const;



		// --- these may set bad() status


		// Check if the existing file can be fopen'ed with "rb", or the directory has read perms.
		// Note: This function should be use only as an utility function (e.g. for GUI notification);
		// other uses are not logically concurrent-safe (therefore, insecure).
		// bad() status is set on failure.
		inline bool is_readable();

		// Check if the existing or soon to be created file is writable, or if files can be created in this dir.
		// Note: The same security considerations apply to this function as to is_readable().
		// bad() status is set on failure.
		inline bool is_writable();


		// Check if anything exists at this path.
		// Note: The same security considerations apply to this function as to is_readable().
		// bad() status is set on failure.
		inline bool exists();

		// Check if it's a file (any type, including block, etc..,). Will also match for symlinks to files.
		// Note: The same security considerations apply to this function as to is_readable().
		// bad() status is set on failure.
		inline bool is_file();

		// Check if it's a regular file. Will also match for symlinks to files.
		// Note: The same security considerations apply to this function as to is_readable().
		// bad() status is set on failure.
		inline bool is_regular();

		// Check if it's a directory.
		// Note: The same security considerations apply to this function as to is_readable().
		// bad() status is set on failure.
		inline bool is_dir();

		// Check if it's a symlink.
		// Note: The same security considerations apply to this function as to is_readable().
		// bad() status is set on failure.
		inline bool is_symlink();

		// If current path is a symbolic link, put its destination into dest.
		// Note: You should avoid calling this in loop, you may end up with an
		// infinite loop if the links point to each other.
		// Note: The returned path may be relative to the link's directory.
		// If error occurs, false is returned and bad() status is set.
		// If current path is not a symbolic link, false is returned, bad() is _not_ set.
		// If false is returned, dest is untouched.
		inline bool get_link_destination(std::string& dest);

		// Do NOT assign the result to int - time_t is implementation-defined.
		// Usually it's seconds since epoch, see time(2) for details.
		// bad() status is set on failure and false is returned.
		inline bool get_last_modified(time_t& put_here);

		// Set the last modified time. It will also change the last access
		// time as a side effect.
		// bad() status is set on failure and false is returned.
		inline bool set_last_modified(time_t t);


		// Create a directory (assuming that the parent directory already exists).
		// Note: If creating with parents, when failing to create one of the directories,
		// it won't remove the previously created ones.
		// Note: If creating with parents, the supplied path _must_ be absolute.
		// bad() status is set on failure and false is returned.
		inline bool make_dir(mode_t octal_mode, bool with_parents);


		// Remove a file or directory.
		// bad() status is set on failure and false is returned.
		inline bool remove(bool recursive = false);


	protected:

		std::string path_;  // current path

};





// ------------------------------------------- Implementation



// Convert path from unknown format to native (e.g. unix paths to win32).
// The current object is also modified.
inline FsPath& FsPath::to_native()
{
	path_ = path_to_native(path_);
	return *this;
}


// Remove trailing separators in path (unless they are parts of root component).
// The current object is also modified.
inline FsPath& FsPath::trim_trailing()
{
	path_ = path_trim_trailing_separators(path_);
	return *this;
}


// Go up "steps" steps. The current object is also modified.
inline FsPath& FsPath::go_up(unsigned int steps)
{
	while (steps--)
		path_ = get_dirname();
	return *this;
}


inline FsPath& FsPath::append(const std::string& partial_path)
{
	// trim leading separators of partial_path
	std::string::size_type index = partial_path.find_first_not_of(DIR_SEPARATOR);
	if (index != std::string::npos) {  // if no non-separator characters found, do nothing.
		trim_trailing();
		if (!is_root())
			path_ += DIR_SEPARATOR_S;
		path_ += partial_path.substr(index);
	}
	return *this;
}


// Compress a path - remove double separators, trailing
// separator, "/./" components, and deal with "/../" if possible.
inline FsPath& FsPath::compress()
{
	path_ = path_compress(path_);
	return *this;
}


// get the path truncated by 1 level, e.g. /usr/local/ -> /usr.
inline std::string FsPath::get_dirname() const
{
	return path_get_dirname(path_);
}


// get the basename of path, e.g. /usr/local/ -> local; /a/b -> b.
inline std::string FsPath::get_basename() const
{
	return path_get_basename(path_);
}


// Get root path of current path. e.g. '/' or 'D:\'.
// May not work with relative paths under win32.
inline std::string FsPath::get_root() const
{
	return path_get_root(path_);
}


// Check if the path corresponds to root (drive / share in win32).
inline bool FsPath::is_root() const
{
	std::string p = path_trim_trailing_separators(path_);
	return (path_is_absolute(p) == p.size());
}


// Get an extension of the last component.
inline std::string FsPath::get_extension() const
{
	std::string base = get_basename();
	std::string::size_type pos = base.rfind('.');
	if (pos != std::string::npos)
		return base.substr(pos + 1);
	return std::string();
}


// check if the path is absolute (only for native paths). returns 0 if it's not.
// the returned value is a position past the root component (e.g. 3 for C:\temp).
inline std::string::size_type FsPath::is_absolute() const
{
	return path_is_absolute(path_);
}


// check if the current path is a subpath of supplied argument.
// e.g. /usr/local/bin is a subpath of /usr. Note: This doesn't check real paths, only strings.
inline bool FsPath::is_subpath_of(const std::string& superpath) const
{
	return (path_.compare(0, superpath.length(), superpath) == 0);
}





// Check if the existing file can be fopen'ed with "rb", or the directory has read perms.
// Note: This function should be use only as an utility function (e.g. for GUI notification);
// other uses are not concurrent-safe (therefore, insecure).
inline bool FsPath::is_readable()
{
	clear_error();

	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to check if a file or directory is readable: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

	if (access(path_.c_str(), R_OK) == -1)  // from unistd.h. works with reading on win32 too.
		set_error(HZ__("File or directory \"/path1/\" is not readable: /errno/."), errno, path_);

	return ok();
}



// Check if the existing or soon to be created file is writable, or if files can be created in this dir.
// Note: The same security considerations apply to this function as to is_readable().
// bad() status is set on failure.
inline bool FsPath::is_writable()
{
	clear_error();
	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to check if a file or directory is writable: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

	bool is_directory = is_dir();
	bool path_exists = exists();
	std::string dirname = (is_directory ? path_trim_trailing_separators(path_) : get_dirname());

	clear_error();

#ifdef _WIN32  // win32 doesn't get access() (it just doesn't work with writing)

	// If it doesn't exist, try to create it.
	// If it exists and is a file, try to open it for writing.
	// If it exists and is a directory, try to create a test file in it.
	// Note: This method is possibly non-suitable for symlink-capable filesystems.

	std::string path_to_check = path_trim_trailing_separators(path_);
	if (path_exists && is_directory) {
		path_to_check += std::string(DIR_SEPARATOR_S) + "__test.txt";
		path_exists = FsPath(path_to_check).exists();
	}

	// path_to_check either doesn't exist, or it's a file. try to open it.
	FILE* f = fopen(path_to_check.c_str(), "ab");   // this creates a 0 size file if it doesn't exist!
	if (!f) {
		set_error(HZ__("File or directory \"/path1/\" is not writable: /errno/."), errno, path_);
		return false;
	}

	if (fclose(f) != 0) {
		set_error(std::string(HZ__("Unable to check if a file or directory \"/path1/\" is writable: "))
				+ HZ__("Error while closing file: /errno/."), errno, path_);
		return false;
	}

	// remove the created file
	if (path_exists && unlink(path_to_check.c_str()) == -1) {
		set_error(std::string(HZ__("Unable to check if a file or directory \"/path1/\" is writable: "))
				+ HZ__("Error while removing file: /errno/."), errno, path_);
		return false;
	}

	// All OK

#else

	if (path_exists && is_directory) {
		if (access(dirname.c_str(), W_OK) == -1) {
			set_error(HZ__("File or directory \"/path1/\" is not writable: /errno/."), errno, path_);
		}

	} else {  // no such path or it's a file
		if (path_exists) {  // checking an existing file
			if (access(path_.c_str(), W_OK) == -1)
				set_error(HZ__("File or directory \"/path1/\" is not writable: /errno/."), errno, path_);

		} else {  // no such path, check parent dir's access mode
			if (access(dirname.c_str(), W_OK) == -1)
				set_error(HZ__("File or directory \"/path1/\" is not writable: /errno/."), errno, path_);
		}
	}

#endif

	return ok();
}




// Check if anything exists at this path.
// Note: The same security considerations apply to this function as to is_readable().
// bad() status is set on failure.
inline bool FsPath::exists()
{
	clear_error();
	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to check if a file or directory exists: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

	if (access(path_.c_str(), F_OK) == -1) {
		if (errno != ENOENT)  // ENOENT (No such file or directory) shouldn't be reported as error.
			set_error(HZ__("File or directory \"/path1/\" doesn't exist: /errno/."), errno, path_);
		return false;
	}
	return ok();
}


// Check if it's a file (any type, including block, etc..,). Will also match for symlinks to files.
// Note: The same security considerations apply to this function as to is_readable().
// bad() status is set on failure.
inline bool FsPath::is_file()
{
	clear_error();
	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to check if a path points to a file: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

	struct stat s;
	if (stat(path_.c_str(), &s) == -1) {
		set_error(HZ__("Unable to check if a path \"/path1/\" points to a file: /errno/."), errno, path_);
		return false;
	}

	if (S_ISDIR(s.st_mode)) {  // we check for dir. anything else is a file.
// 		set_error("The path \"" + path_ + "\" points to directory.");
		return false;
	}

	return true;
}


// Check if it's a regular file. Will also match for symlinks to files.
// Note: The same security considerations apply to this function as to is_readable().
// bad() status is set on failure.
inline bool FsPath::is_regular()
{
	clear_error();
	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to check if a path points to a regular file: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

	struct stat s;
	if (stat(path_.c_str(), &s) == -1) {
		set_error(HZ__("Unable to check if a path \"/path1/\" points to a regular file: /errno/."), errno, path_);
		return false;
	}

	if (!S_ISREG(s.st_mode)) {
// 		set_error("The path \"" + path_ + "\" points to a non-regular file.");
		return false;
	}

	return true;
}


// Check if it's a directory.
// Note: The same security considerations apply to this function as to is_readable().
// bad() status is set on failure.
inline bool FsPath::is_dir()
{
	clear_error();
	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to check if a path points to directory: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

	struct stat s;
	if (stat(path_.c_str(), &s) == -1) {
		set_error(HZ__("Unable to check if a path \"/path1/\" points to directory: /errno/."), errno, path_);
		return false;
	}

	if (!S_ISDIR(s.st_mode)) {
// 		set_error("The path \"" + path_ + "\" points to a file.");
		return false;
	}

	return true;
}


// Check if it's a symlink.
// Note: The same security considerations apply to this function as to is_readable().
// bad() status is set on failure.
inline bool FsPath::is_symlink()
{
	clear_error();
	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to check if a path points to a symbolic link: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

#ifdef _WIN32
	// well, win32 and all...
	return false;

#else
	struct stat s;
	if (lstat(path_.c_str(), &s) == -1) {
		set_error(HZ__("Unable to check if a path \"/path1/\" points to a symbolic link: /errno/."), errno, path_);
		return false;
	}

	if (!S_ISLNK(s.st_mode)) {
		return false;
	}

	return true;
#endif
}



// If current path is a symbolic link, put its destination into dest.
// Note: You should avoid calling this in loop, you may end up with an
// infinite loop if the links point to each other.
// Note: The returned path may be relative to the link's directory.
// If error occurs, false is returned and bad() status is set.
// If current path is not a symbolic link, false is returned, bad() is _not_ set.
// If false is returned, dest is untouched.
inline bool FsPath::get_link_destination(std::string& dest)
{
	clear_error();
	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to get link destination: ")) + HZ__("Supplied path is empty."));
		return false;
	}

#ifdef _WIN32
	return false;  // not a link

#else

	std::size_t buf_size = 256;  // size_t, as accepted by readlink().

	do {
		char* buf = new char[buf_size];

		// readlink() returns the number of written bytes, not including terminating 0.
		ssize_t written = readlink(path_.c_str(), buf, buf_size);

		if (written == -1) {  // error
			if (errno != EINVAL) {  // EINVAL: The named file is not a symbolic link.
				set_error(HZ__("Unable to get link destination of path \"/path1/\": /errno/."), errno, path_);
			}
			delete[] buf;
			return false;
		}

		if (written != static_cast<ssize_t>(buf_size)) {  // means we have enough room
			buf[written] = '\0';  // there's a place for this.
			dest = buf;
			delete[] buf;
			break;

		} else {  // doesn't fit, increase size.
			buf_size *= 4;
		}

		delete[] buf;

	} while (true);

	return true;
#endif
}




inline bool FsPath::get_last_modified(time_t& put_here)
{
	clear_error();
	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to get the last modification time of a path: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

	struct stat s;
	if (stat(path_.c_str(), &s) == -1) {
		set_error(HZ__("Unable to get the last modification time of path \"/path1/\": /errno/."), errno, path_);
		return false;
	}

	put_here = s.st_mtime;

	return ok();
}



inline bool FsPath::set_last_modified(time_t t)
{
	clear_error();
	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to set the last modification time of a filesystem entry: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

	struct utimbuf tb;
	tb.actime = t;  // this is a side effect - change access time too
	tb.modtime = t;

	if (utime(path_.c_str(), &tb) == -1) {
		set_error(HZ__("Unable to set the last modification time of path \"/path1/\": /errno/."), errno, path_);
		return false;
	}

	return ok();
}




// Create a directory.
inline bool FsPath::make_dir(mode_t octal_mode, bool with_parents)
{
	clear_error();
	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to create directory: ")) + HZ__("Supplied path is empty."));
		return false;
	}

	if (!with_parents) {

#ifdef _WIN32
		int status = mkdir(path_.c_str());
#else
		int status = mkdir(path_.c_str(), octal_mode);
#endif
		if (status == -1) {
			set_error(HZ__("Unable to create directory at path \"/path1/\": /errno/."), errno, path_);
			return false;
		}


	} else {  // with parents:

		if (!is_absolute()) {
			set_error(std::string(HZ__("Unable to create directory with parents: "))
					+ HZ__("Supplied path must be absolute."));
			return false;
		}

		std::string::size_type curr = 0, last = 0, end = path_.size();
		std::string last_created = get_root();

		while (true) {
			if (last >= end)  // last is past the end
				break;
			curr = path_.find(DIR_SEPARATOR, last);

			if (curr != last) {
				last_created += (DIR_SEPARATOR + path_.substr(last, (curr == std::string::npos ? curr : (curr - last))));
				FsPath p(last_created);
				if (!p.make_dir(octal_mode, false)) {  // try to create
					this->import_error(p);
					break;
				}
			}

			if (curr == std::string::npos)
				break;
			last = curr + 1;
		}
	}

	return ok();
}




namespace internal {

	// Helper function, internal.
	// returns the number of not removed files. 0 on success. pass directory only.
	inline int path_remove_dir_recursive(const std::string& path)
	{
		hz::FsPath p(path);
		if (!p.exists())
			return 1;  // couldn't remove 1 file. maybe doesn't exist - it's still an error.

	// 	if (! p.is_file().bad()) {  // file
	// 		if (unlink(path.c_str()) == -1)
	// 			return 1;
	// 		return 0;
	// 	}

		int error_count = 0;

		// It's a directory, iterate it, then remove it
		DIR* dir = opendir(path.c_str());

		if (dir) {
			while (true) {
				errno = 0;
				struct dirent* entry = readdir(dir);

				if (errno) {  // error while reading entry. try next one.
					error_count++;
					continue;
				}
				if (!entry) {  // no error and entry is null - means we reached the end.
					break;
				}

				std::string entry_name = entry->d_name;
				if (entry_name == "." || entry_name == "..")  // won't delete those
					continue;

				std::string entry_path = path + DIR_SEPARATOR_S + entry->d_name;
				FsPath ep(entry_path);

				if (ep.is_dir() && !ep.is_symlink()) {  // if it's a directory and not a symlink, go recursive
					error_count += path_remove_dir_recursive(entry_path);

				} else {  // just remove it
					if (::unlink(entry_path.c_str()) == -1)
						error_count++;
				}
			}

			closedir(dir);
		}

		// try to remove dir even if it's non-readable.
		if (::rmdir(path.c_str()) == -1)
			return ++error_count;

		return error_count;
	}

}  // ns




// Remove file or directory.
inline bool FsPath::remove(bool recursive)
{
	clear_error();
	if (path_.empty()) {
		set_error(std::string(HZ__("Unable to remove file or directory: ")) + HZ__("Supplied path is empty."));
		return false;
	}

	if (path_trim_trailing_separators(path_) == get_root()) {
		set_error(std::string(HZ__("Unable to remove file or directory \"/path1/\": "))
				+ HZ__("Cannot remove root directory."), 0, path_);
		return false;
	}

	if (recursive && !is_file()) {
		clear_error();  // clear previous function call
		if (internal::path_remove_dir_recursive(path_) > 0) {  // supply dirs here only.
			set_error(HZ__("Unable to remove directory \"/path1/\" completely: Some files couldn't be deleted."), 0, path_);
		}
		return false;
	}

#ifdef _WIN32  // win2k (maybe later wins too) remove() says "permission denied" (!) on directories.
	int status = 0;
	if (is_dir()) {
		status = ::rmdir(path_.c_str());  // empty dir only

	} else {
		status = ::unlink(path_.c_str());  // files only
	}

	if (status == -1) {
		set_error(HZ__("Unable to remove file or directory \"/path1/\": /errno/."), errno, path_);
	}

#else
	if (::remove(path_.c_str()) == -1) {

		// In case of not empty directory, POSIX says the error will be EEXIST
		// or ENOTEMPTY. Linux uses ENOTEMPTY, Solaris uses EEXIST.
		// ENOTEMPTY makes more sense for error messages, so convert.
		if (errno == EEXIST)
			errno = ENOTEMPTY;

		set_error(HZ__("Unable to remove file or directory \"/path1/\": /errno/."), errno, path_);
	}
#endif

	return ok();
}





}  // ns hz



#endif
