/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_FS_PATH_H
#define HZ_FS_PATH_H

#include "hz_config.h"  // feature macros

#include <string>
#include <cerrno>  // errno (not std::errno, it may be a macro)
#include <cstdio>  // for stdio.h; std::FILE, std::fclose, std::remove.
#include <stdio.h>  // _wfopen*
#include <ctime>  // std::time_t
#include <sys/types.h>  // *stat() needs this; utime.h needs this; mode_t
#include <sys/stat.h>  // *stat() needs this; mkdir()

#ifdef _WIN32
	#include <io.h>  // _waccess*(), _wstat(), _wunlink(), _wrmdir(), _wmkdir()
	#include <utime.h>  // _wutime()
#else
	#include <cstddef>  // std::size_t
	#include <unistd.h>  // access(), stat(), unlink(), readlink()
	#include <utime.h>  // utime()
#endif

#include "fs_common.h"  // separator
#include "fs_path_utils.h"  // path_* functions
#include "fs_error_holder.h"  // FsErrorHolder
#include "fs_dir_platform.h"  // directory_* functions
#include "win32_tools.h"  // hz::win32_* charset conversion


// Filesystem path and file manipulation.

// This API accepts/gives utf-8 filenames/paths on win32,
// current locale filenames/paths on others (just like glib).



namespace hz {




// The sole purpose of this class is to enforce privacy of its members.
// You can not construct an object of this class.
class FsPathHolder {

	protected:  // construct only from children

		FsPathHolder()
#ifdef _WIN32
				: utf16_path_(0)
#endif
		{ }

		// you should check the success status with bad().
		FsPathHolder(const std::string& path) : path_(path)
#ifdef _WIN32
				, utf16_path_(0)
#endif
		{ }


	public:

		virtual ~FsPathHolder()
		{
#ifdef _WIN32
			delete[] utf16_path_;
#endif
		}


		// --- these will _not_ set bad() status

		void set_path(const std::string& path)
		{
			path_ = path;
#ifdef _WIN32
			delete[] utf16_path_;
			utf16_path_ = 0;  // reset
#endif
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

		bool empty() const
		{
			return path_.empty();
		}


		// These are sort-of internal
#ifdef _WIN32
		// Note: This may actually return 0 if it's unsupported or the encoded string is invalid.
		const wchar_t* get_utf16() const
		{
			if (!utf16_path_)
				utf16_path_ = hz::win32_utf8_to_utf16(path_.c_str());
			return utf16_path_;
		}

		bool set_utf16(const wchar_t* path)
		{
			char* utf8 = win32_utf16_to_utf8(path);
			if (utf8) {
				set_path(utf8);  // this correctly resets utf16_path_
				delete[] utf8;
				return true;
			}
			return false;
		}
#endif


	private:  // don't let children modify path_ directly, it will desync utf16_path_.

		std::string path_;  // current path. always utf-8 in windows.

#ifdef _WIN32
		mutable wchar_t* utf16_path_;
#endif

};







class FsPath : public FsPathHolder, public FsErrorHolder {

	public:

#ifdef _WIN32
		typedef _mode_t mode_type;  // they have some kind of unhealthy love for underscores.
#else
		typedef mode_t mode_type;
#endif

		FsPath()
		{ }

		// you should check the success status with bad().
		FsPath(const std::string& path) : FsPathHolder(path)
		{ }

		virtual ~FsPath()
		{ }



		// --- these will _not_ set bad() status


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
		// bad() status is set on error.
		inline bool is_file();

		// Check if it's a regular file. Will also match for symlinks to files.
		// Note: The same security considerations apply to this function as to is_readable().
		// bad() status is set on failure.
		inline bool is_regular();

		// Check if it's a directory.
		// Note: The same security considerations apply to this function as to is_readable().
		// bad() status is set on error.
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

		// Do NOT assign the result to int - std::time_t is implementation-defined.
		// Usually it's seconds since epoch, see time(2) for details.
		// bad() status is set on failure and false is returned.
		inline bool get_last_modified(std::time_t& put_here);

		// Set the last modified time. It will also change the last access
		// time as a side effect.
		// bad() status is set on failure and false is returned.
		inline bool set_last_modified(std::time_t t);


		// Create a directory (assuming that the parent directory already exists).
		// octal_mode parameter is ignored on Windows.
		// Note: If creating with parents, when failing to create one of the directories,
		// it won't remove the previously created ones.
		// Note: If creating with parents, the supplied path _must_ be absolute.
		// bad() status is set on failure and false is returned.
		inline bool make_dir(mode_type octal_mode, bool with_parents);


		// Remove a file or directory.
		// bad() status is set on failure and false is returned.
		inline bool remove(bool recursive = false);


};





// ------------------------------------------- Implementation



// Convert path from unknown format to native (e.g. unix paths to win32).
// The current object is also modified.
inline FsPath& FsPath::to_native()
{
	this->set_path(path_to_native(this->get_path()));
	return *this;
}


// Remove trailing separators in path (unless they are parts of root component).
// The current object is also modified.
inline FsPath& FsPath::trim_trailing()
{
	this->set_path(path_trim_trailing_separators(this->get_path()));
	return *this;
}


// Go up "steps" steps. The current object is also modified.
inline FsPath& FsPath::go_up(unsigned int steps)
{
	std::string p = this->get_path();
	while (steps--)
		p = get_dirname();
	this->set_path(p);
	return *this;
}


inline FsPath& FsPath::append(const std::string& partial_path)
{
	std::string p = this->get_path();
	// trim leading separators of partial_path
	std::string::size_type index = partial_path.find_first_not_of(DIR_SEPARATOR);
	if (index != std::string::npos) {  // if no non-separator characters found, do nothing.
		trim_trailing();
		if (!is_root())
			p += DIR_SEPARATOR_S;
		p += partial_path.substr(index);
	}
	this->set_path(p);
	return *this;
}


// Compress a path - remove double separators, trailing
// separator, "/./" components, and deal with "/../" if possible.
inline FsPath& FsPath::compress()
{
	this->set_path(path_compress(this->get_path()));
	return *this;
}


// get the path truncated by 1 level, e.g. /usr/local/ -> /usr.
inline std::string FsPath::get_dirname() const
{
	return path_get_dirname(this->get_path());
}


// get the basename of path, e.g. /usr/local/ -> local; /a/b -> b.
inline std::string FsPath::get_basename() const
{
	return path_get_basename(this->get_path());
}


// Get root path of current path. e.g. '/' or 'D:\'.
// May not work with relative paths under win32.
inline std::string FsPath::get_root() const
{
	return path_get_root(this->get_path());
}


// Check if the path corresponds to root (drive / share in win32).
inline bool FsPath::is_root() const
{
	std::string p = path_trim_trailing_separators(this->get_path());
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
	return path_is_absolute(this->get_path());
}


// check if the current path is a subpath of supplied argument.
// e.g. /usr/local/bin is a subpath of /usr. Note: This doesn't check real paths, only strings.
inline bool FsPath::is_subpath_of(const std::string& superpath) const
{
	return (this->get_path().compare(0, superpath.length(), superpath) == 0);
}





// Check if the existing file can be fopen'ed with "rb", or the directory has read perms.
// Note: This function should be use only as an utility function (e.g. for GUI notification);
// other uses are not concurrent-safe (therefore, insecure).
inline bool FsPath::is_readable()
{
	clear_error();

	if (this->empty()) {
		set_error(std::string(HZ__("Unable to check if a file or directory is readable: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

#if defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
	if (_waccess_s(this->get_utf16(), 04))  // msvc uses integers instead (R_OK == 04 anyway).
#elif defined _WIN32
	if (_waccess(this->get_utf16(), 04) == -1)  // *access*() may not work with < win2k with directories.
#else
	if (access(this->c_str(), R_OK) == -1)  // from unistd.h
#endif
	{
		set_error(HZ__("File or directory \"/path1/\" is not readable: /errno/."), errno, this->get_path());
	}

	return ok();
}



// Check if the existing or soon to be created file is writable, or if files can be created in this dir.
// Note: The same security considerations apply to this function as to is_readable().
// bad() status is set on failure.
inline bool FsPath::is_writable()
{
	clear_error();
	if (this->empty()) {
		set_error(std::string(HZ__("Unable to check if a file or directory is writable: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

	bool is_directory = is_dir();
	bool path_exists = exists();
	std::string dirname = (is_directory ? path_trim_trailing_separators(this->get_path()) : get_dirname());

	clear_error();

#ifdef _WIN32  // win32 doesn't get access() (it just doesn't work with writing)

	// If it doesn't exist, try to create it.
	// If it exists and is a file, try to open it for writing.
	// If it exists and is a directory, try to create a test file in it.
	// Note: This method is possibly non-suitable for symlink-capable filesystems.

	FsPath path_to_check(this->get_path());
	path_to_check.trim_trailing();
	if (path_exists && is_directory) {
		path_to_check.set_path(path_to_check.get_path() += std::string(DIR_SEPARATOR_S) + "__test.txt");
		path_exists = path_to_check.exists();
	}

	// pcheck either doesn't exist, or it's a file. try to open it.
	std::FILE* f = 0;
#if defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
	errno = _wfopen_s(&f, path_to_check.get_utf16(), L"ab");
#else
	f = _wfopen(path_to_check.get_utf16(), L"ab");   // this creates a 0 size file if it doesn't exist!
#endif
	if (!f) {
		set_error(HZ__("File or directory \"/path1/\" is not writable: /errno/."), errno, this->get_path());
		return false;
	}

	if (std::fclose(f) != 0) {
		set_error(std::string(HZ__("Unable to check if a file or directory \"/path1/\" is writable: "))
				+ HZ__("Error while closing file: /errno/."), errno, this->get_path());
		return false;
	}

	// remove the created file
	if (path_exists && _wunlink(path_to_check.get_utf16()) == -1) {
		set_error(std::string(HZ__("Unable to check if a file or directory \"/path1/\" is writable: "))
				+ HZ__("Error while removing file: /errno/."), errno, this->get_path());
		return false;
	}

	// All OK

#else

	if (path_exists && is_directory) {
		if (access(dirname.c_str(), W_OK) == -1) {
			set_error(HZ__("File or directory \"/path1/\" is not writable: /errno/."), errno, this->get_path());
		}

	} else {  // no such path or it's a file
		if (path_exists) {  // checking an existing file
			if (access(this->c_str(), W_OK) == -1)
				set_error(HZ__("File or directory \"/path1/\" is not writable: /errno/."), errno, this->get_path());

		} else {  // no such path, check parent dir's access mode
			if (access(dirname.c_str(), W_OK) == -1)
				set_error(HZ__("File or directory \"/path1/\" is not writable: /errno/."), errno, this->get_path());
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
	if (this->empty()) {
		set_error(std::string(HZ__("Unable to check if a file or directory exists: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

#if defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
	if (_waccess_s(this->get_utf16(), 00) == -1)  // msvc uses integers instead (F_OK == 00 anyway).
#elif defined _WIN32
	if (_waccess(this->get_utf16(), 00) == -1)  // msvc uses integers instead (F_OK == 00 anyway).
#else
	if (access(this->c_str(), F_OK) == -1)
#endif
	{
		if (errno != ENOENT)  // ENOENT (No such file or directory) shouldn't be reported as error.
			set_error(HZ__("File or directory \"/path1/\" doesn't exist: /errno/."), errno, this->get_path());
		return false;
	}
	return ok();
}


// Check if it's a file (any type, including block, etc..,). Will also match for symlinks to files.
// Note: The same security considerations apply to this function as to is_readable().
// bad() status is set on error.
inline bool FsPath::is_file()
{
	clear_error();
	if (this->empty()) {
		set_error(std::string(HZ__("Unable to check if a path points to a file: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

#ifdef _WIN32
	struct _stat s;
	const int stat_result = _wstat(this->get_utf16(), &s);
#else
	struct stat s;
	const int stat_result = stat(this->c_str(), &s);
#endif
	if (stat_result == -1) {
		set_error(HZ__("Unable to check if a path \"/path1/\" points to a file: /errno/."), errno, this->get_path());
		return false;
	}

#ifdef _WIN32
	if (s.st_mode & _S_IFDIR)
#else
	if (S_ISDIR(s.st_mode))  // we check for dir. anything else is a file.
#endif
	{
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
	if (this->empty()) {
		set_error(std::string(HZ__("Unable to check if a path points to a regular file: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

#ifdef _WIN32
	struct _stat s;
	const int stat_result = _wstat(this->get_utf16(), &s);
#else
	struct stat s;
	const int stat_result = stat(this->c_str(), &s);
#endif
	if (stat_result == -1) {
		set_error(HZ__("Unable to check if a path \"/path1/\" points to a regular file: /errno/."), errno, this->get_path());
		return false;
	}

#ifdef _WIN32
	// MS documentation contradicts itself about when _S_IFREG is set
	// (see _fstat() and _stat()). _stat() says it's a regular file or a char device,
	// _fstat() says it's a regular file and char device is represented by _S_IFCHR.
	// We check both, just in case.
	if (!(s.st_mode & _S_IFREG) || (s.st_mode & _S_IFCHR))  // if it's not a regular file or it's a char device.
#else
	if (!S_ISREG(s.st_mode))  // we check for dir. anything else is a file.
#endif
	{
// 		set_error("The path \"" + path_ + "\" points to a non-regular file.");
		return false;
	}

	return true;
}


// Check if it's a directory.
// Note: The same security considerations apply to this function as to is_readable().
// bad() status is set on error.
inline bool FsPath::is_dir()
{
	clear_error();
	if (this->empty()) {
		set_error(std::string(HZ__("Unable to check if a path points to directory: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

#ifdef _WIN32
	struct _stat s;
	const int stat_result = _wstat(this->get_utf16(), &s);
#else
	struct stat s;
	const int stat_result = stat(this->c_str(), &s);
#endif
	if (stat_result == -1) {
		set_error(HZ__("Unable to check if a path \"/path1/\" points to directory: /errno/."), errno, this->get_path());
		return false;
	}

#ifdef _WIN32
	if (!(s.st_mode & _S_IFDIR))
#else
	if (!S_ISDIR(s.st_mode))
#endif
	{
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
	if (this->empty()) {
		set_error(std::string(HZ__("Unable to check if a path points to a symbolic link: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

#ifdef _WIN32
	// well, win32 and all...
	// Although there are symlinks there (theoretically), we don't want to get
	// our hands dirty (glib doesn't).
	return false;

#else
	struct stat s;
	if (lstat(this->c_str(), &s) == -1) {
		set_error(HZ__("Unable to check if a path \"/path1/\" points to a symbolic link: /errno/."), errno, this->get_path());
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
	if (this->empty()) {
		set_error(std::string(HZ__("Unable to get link destination: ")) + HZ__("Supplied path is empty."));
		return false;
	}

#ifdef _WIN32
	return false;  // not a link (see is_symlink())

#else

	std::size_t buf_size = 256;  // size_t, as accepted by readlink().

	do {
		char* buf = new char[buf_size];

		// readlink() returns the number of written bytes, not including terminating 0.
		ssize_t written = readlink(this->c_str(), buf, buf_size);

		if (written == -1) {  // error
			if (errno != EINVAL) {  // EINVAL: The named file is not a symbolic link.
				set_error(HZ__("Unable to get link destination of path \"/path1/\": /errno/."), errno, this->get_path());
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




inline bool FsPath::get_last_modified(std::time_t& put_here)
{
	clear_error();
	if (this->empty()) {
		set_error(std::string(HZ__("Unable to get the last modification time of a path: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

#ifdef _WIN32
	struct _stat s;
	const int stat_result = _wstat(this->get_utf16(), &s);
#else
	struct stat s;
	const int stat_result = stat(this->c_str(), &s);
#endif
	if (stat_result == -1) {
		set_error(HZ__("Unable to get the last modification time of path \"/path1/\": /errno/."), errno, this->get_path());
		return false;
	}

	put_here = s.st_mtime;

	return ok();
}



inline bool FsPath::set_last_modified(std::time_t t)
{
	clear_error();
	if (this->empty()) {
		set_error(std::string(HZ__("Unable to set the last modification time of a filesystem entry: "))
				+ HZ__("Supplied path is empty."));
		return false;
	}

#ifdef _WIN32
	struct _utimbuf tb;
#else
	struct utimbuf tb;
#endif
	tb.actime = t;  // this is a side effect - change access time too
	tb.modtime = t;

#ifdef _WIN32
	if (_wutime(this->get_utf16(), &tb) == -1)
#else
	if (utime(this->c_str(), &tb) == -1)
#endif
	{
		set_error(HZ__("Unable to set the last modification time of path \"/path1/\": /errno/."), errno, this->get_path());
		return false;
	}

	return ok();
}




// Create a directory. Returns true if successful or if the directory already exists.
// octal_mode parameter is ignored on Windows.
inline bool FsPath::make_dir(mode_type octal_mode, bool with_parents)
{
	// debug_out_dump("hz", DBG_FUNC_MSG << "Path: \"" << path_ << "\"\n");
	clear_error();
	if (this->empty()) {
		set_error(std::string(HZ__("Unable to create directory: ")) + HZ__("Supplied path is empty."));
		return false;
	}

	if (is_root())
		return true;

	if (with_parents) {
		if (!is_absolute()) {
			set_error(std::string(HZ__("Unable to create directory with parents at path \"/path1/\": "))
					+ HZ__("Supplied path must be absolute."), 0, this->get_path());
			return false;
		}

		FsPath p(get_dirname());
		if (!p.make_dir(octal_mode, with_parents)) {
			import_error(p);
			return false;
		}
	}

#ifdef _WIN32
	int status = _wmkdir(this->get_utf16());
#else
	int status = mkdir(this->c_str(), octal_mode);
#endif
	if (status == -1) {
		if (errno == EEXIST && is_dir()) {
			return true;  // already exists
		}
		set_error(HZ__("Unable to create directory at path \"/path1/\": /errno/."), errno, this->get_path());
		return false;
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

// 		if (! p.is_file().bad()) {  // file
// #ifdef _WIN32
// 			return static_cast<int>(_wunlink(p.get_utf16()) == -1);
// #else
// 			return static_cast<int>(unlink(p.c_str()) == -1);
// #endif
// 		}

		int error_count = 0;

		// It's a directory, iterate it, then remove it
		directory_handle_type dir = directory_open(path.c_str());

		if (dir) {
			while (true) {
				errno = 0;
				directory_entry_handle_type entry = directory_read(dir);

				if (errno) {  // error while reading entry. try next one.
					error_count++;
					continue;
				}
				if (!entry) {  // no error and entry is null - means we reached the end.
					break;
				}

				std::string entry_name = directory_entry_name(entry);
				if (entry_name == "." || entry_name == "..")  // won't delete those
					continue;

				std::string entry_path = path + DIR_SEPARATOR_S + directory_entry_name(entry);
				FsPath ep(entry_path);

				if (ep.is_dir() && !ep.is_symlink()) {  // if it's a directory and not a symlink, go recursive
					error_count += path_remove_dir_recursive(entry_path);

				} else {  // just remove it
#ifdef _WIN32
					if (_wunlink(ep.get_utf16()) == -1)
#else
					if (unlink(ep.c_str()) == -1)
#endif
					{
						error_count++;
					}
				}
			}

			directory_close(dir);
		}

		// try to remove dir even if it's non-readable.
#ifdef _WIN32
		const int rmdir_result = _wrmdir(p.get_utf16());
#else
		const int rmdir_result = rmdir(p.c_str());
#endif
		if (rmdir_result == -1)
			return ++error_count;

		return error_count;
	}


}  // ns




// Remove file or directory.
inline bool FsPath::remove(bool recursive)
{
	clear_error();
	if (this->empty()) {
		set_error(std::string(HZ__("Unable to remove file or directory: ")) + HZ__("Supplied path is empty."));
		return false;
	}

	if (path_trim_trailing_separators(this->get_path()) == get_root()) {
		set_error(std::string(HZ__("Unable to remove file or directory \"/path1/\": "))
				+ HZ__("Cannot remove root directory."), 0, this->get_path());
		return false;
	}

	if (recursive && !is_file()) {
		clear_error();  // clear previous function call
		if (internal::path_remove_dir_recursive(this->get_path()) > 0) {  // supply dirs here only.
			set_error(HZ__("Unable to remove directory \"/path1/\" completely: Some files couldn't be deleted."), 0, this->get_path());
		}
		return false;
	}

#ifdef _WIN32  // win2k (maybe later wins too) remove() says "permission denied" (!) on directories.
	int status = 0;
	if (is_dir()) {
		status = _wrmdir(this->get_utf16());  // empty dir only

	} else {
		status = _wunlink(this->get_utf16());  // files only
	}

	if (status == -1) {
		set_error(HZ__("Unable to remove file or directory \"/path1/\": /errno/."), errno, this->get_path());
	}

#else
	if (std::remove(this->c_str()) == -1) {

		// In case of not empty directory, POSIX says the error will be EEXIST
		// or ENOTEMPTY. Linux uses ENOTEMPTY, Solaris uses EEXIST.
		// ENOTEMPTY makes more sense for error messages, so convert.
		if (errno == EEXIST)
			errno = ENOTEMPTY;

		set_error(HZ__("Unable to remove file or directory \"/path1/\": /errno/."), errno, this->get_path());
	}
#endif

	return ok();
}





}  // ns hz



#endif
