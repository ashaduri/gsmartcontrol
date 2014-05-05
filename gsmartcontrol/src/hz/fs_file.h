/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_FS_FILE_H
#define HZ_FS_FILE_H

#include "hz_config.h"  // feature macros

/**
\file
This API accepts/gives utf-8 filenames/paths on win32,
current locale filenames/paths on others (just like glib).


Notes on 64-bit file support:

Glibc/Linux:
Glibc/Linux, Solaris and (to some extent) QNX implement "transitional"
interface when used on 32-bit processors.
http://www.gnu.org/software/libc/manual/html_mono/libc.html
ftello() and fseeko() are available in glibc if _LARGEFILE_SOURCE is
defined, which is also enabled by (_XOPEN_SOURCE >= 500 || _GNU_SOURCE).

ftello64() is available in glibc if _LARGEFILE64_SOURCE is defined,
which is enabled with _GNU_SOURCE.

Defining _FILE_OFFSET_BITS=64 replaces fopen() & friends with fopen64()
and friends transparently. If ftello() is available, it's replaced by ftello64() too
(same for fseeko()). Note that this won't make ftello64() available for applications.
Note that the replacement happens on asm level if the compiler supports it,
so, for example, libstdc++'s cstdio will correctly provide fopen64 as std::fopen().
GCC, Sun and Intel support the transparent conversion. If it's unsupported,
glibc falls back to macro-based substitution, which will fail since cstdio
undefines those macros.

Solaris:
http://docs.sun.com/app/docs/doc/817-3946/6mjgmt4mp?l=en&a=view
http://docs.sun.com/app/docs/doc/816-5175/standards-5?l=en&a=view
Solaris seems to have the same rules regarding _LARGEFILE*_SOURCE
and _FILE_OFFSET_BITS as glibc. _LARGEFILE64_SOURCE implies
_LARGEFILE_SOURCE as well.
Both _LARGEFILE*_SOURCE macros are automatically defined if:
* No explicit standards-conforming environment is requested (neither
_POSIX_SOURCE nor _XOPEN_SOURCE is defined and the value of
__STDC__ does not imply standards conformance), OR:
* Extended system interfaces are explicitly requested (__EXTENSIONS__
is defined).

NetBSD:
(_LARGEFILE_SOURCE || _XOPEN_SOURCE >= 500 || _NETBSD_SOURCE)
enables fseeko() and ftello().
If none of the major feature macros are defined, _NETBSD_SOURCE is assumed.
fopen() and friends use 64-bit offsets by default (off_t is always 64 bits). No
*64() functions are present.

OpenBSD:
fseeko() and ftello() are always defined.
fopen() and friends use 64-bit offsets (off_t is always 64 bits). No *64() functions
are present.

FreeBSD:
__POSIX_VISIBLE >= 200112 (enabled by default) enables fseeko() / ftello().
off_t and friends always operate in 64 bits. No *64() functions
are present.

QNX:
There is no _LARGEFILE_SOURCE, fseeko() / ftello() are always available.
_LARGEFILE64_SOURCE enables *64() variants. _FILE_OFFSET_BITS=64
replaces ordinary functions with *64() equivalents (through asm conversion).
_QNX_SOURCE (default) enables everything.

Darwin:
off_t is 64-bit. Seems to be lifted from FreeBSD.

Windows:
All off_t-related functions use long on Windows. There is no fseeko() / ftello().
One has to use _fseeki64() and friends explicitly.
*/

#include <string>
#include <cstddef>  // std::size_t
#include <cstdio>  // std::FILE, std::fopen() and friends (this doesn't break LFS), std::rename()
#include <stdio.h>  // off_t, fileno(), _fileno(), _wfopen(), _wrename()
#include <cerrno>  // errno
#include <sys/types.h>  // *stat*() needs this
#include <sys/stat.h>  // *stat*() needs this; chmod()

#ifdef _WIN32
	#include <io.h>  // _fstat*(), _wunlink(), _wchmod()
#else
	#include <unistd.h>  // stat(), unlink()
#endif

#include "fs_path.h"  // FsPath

#ifdef _WIN32
	#include "scoped_array.h"  // hz::scoped_array
	#include "win32_tools.h"  // hz::win32_utf8_to_utf16
#endif


// Filesystem file manipulation


namespace hz {




/// \typedef file_size_t
/// Offset & size type. may be uint32_t or uint64_t, depending on system and compilation flags.
/// Note: It is usually discouraged to use this type in headers, because the library may be
/// compiled with one size and the application with another. However, this problem is rather
/// limited if using header-only approach, like we do.

#if defined HAVE_POSIX_OFF_T_FUNCS && HAVE_POSIX_OFF_T_FUNCS
	typedef off_t file_size_t;  // off_t is in stdio.h, available in all self-respecting unix systems.
#elif defined HAVE_WIN_LFS_FUNCS && HAVE_WIN_LFS_FUNCS
	typedef __int64 file_size_t;  // ms stuff, _fseeki64() and friends use it.
#else
	typedef long file_size_t;  // fseek() and friends use long. win32 doesn't have off_t.
#endif


/*
TODO

Seekless read support in hz::File. should be able to accept a fixed buffer,
an std::string of max length, optional ticker callback.
e.g.

// callback
inline bool append_func(std::string& put_here, const std::string& chunk, bool last)
{
	put_here += chunk;
	return true;  // continue
}

// user code
void f (hz::File& file)
{
	std::string put_here;
	file.get_contents(put_here, 1024, append_func);  // 3rd param should have a default func.
}

Should have similar thing with static buffers.
*/


/// A class that represents a file. This can be thought of as a
/// wrapper around std::FILE*.
class File : public FsPath {
	public:

		/// Native handle type.
		typedef std::FILE* handle_type;  // std::FILE must be identical to ::FILE in C++.

		/// Constructor
		File() : file_(NULL)
		{ }

		/// Create a File object with path "path". This will NOT open the file.
		File(const std::string& path) : file_(NULL)
		{
			this->set_path(path);
		}

		/// Create a File object with path "path". This will NOT open the file.
		File(const FsPath& path) : file_(NULL)
		{
			this->set_path(path.get_path());
		}

		/// Create a File object and open a file "path" points to.
		/// You should check the success status with bad().
		File(const std::string& path, const std::string& open_mode) : file_(NULL)
		{
			this->set_path(path);
			this->open(open_mode);
		}

		/// Create a File object and open a file "path" points to.
		/// You should check the success status with bad().
		File(const FsPath& path, const std::string& open_mode) : file_(NULL)
		{
			this->set_path(path.get_path());
			this->open(open_mode);
		}


	private:

		// Between move semantics (confusing and error-prone) and denying copying,
		// I choose to deny.

		/// Private copy constructor to deny copying
		File(const File& other);

		/// Private assignment operator to deny copying
		File& operator= (const File& other);


	public:

/*
		// Copy constructor. Move semantics are implemented - the ownership is transferred
		// exclusively.
		File(File& other) : file_(NULL)
		{
			*this = other;  // operator=
		}

		// move semantics, as with copy constructor.
		inline File& operator= (File& other);
*/


		/// Destructor which invokes close() if needed.
		virtual ~File()
		{
			if (file_)
				this->close();
		}


		// --- these may set bad() status


		/// Open the file with open_mode (standard std::fopen open mode).
		inline bool open(const std::string& open_mode);

		/// Close the previously opened file.
		inline bool close();

		/// Get native file handle (obtained using std::fopen() or similar)
		handle_type get_handle()
		{
			return file_;
		}


		/// Get file contents. \c put_data_here will be allocated to whatever
		/// size is needed to contain all the data. The size is written to \c put_size_here.
		/// You must call "delete[] put_data_here" afterwards.
		/// If the file is larger than \c max_size (100M by default), the function refuses to load it.
		/// Note: No additional trailing 0 is written to data!
		/// \return false on failure (error is also set).
		inline bool get_contents(unsigned char*& put_data_here,
				file_size_t& put_size_here, file_size_t max_size = 104857600);

		/// Same as other versions of get_contents(), but puts data into an already
		/// allocated buffer of size \c buf_size.
		/// If the size is insufficient, false is returned and buffer is left untouched.
		/// If any other error occurs, the buffer is left in unspecified state.
		/// Internal usage only: If buf_size is -1, the buffer will be automatically allocated.
		/// TODO: Support files which are not seekable and don't have a size attribute (e.g. /proc/*).
		/// \return false on failure (error is also set).
		inline bool get_contents_noalloc(unsigned char*& put_data_here, file_size_t buf_size,
				file_size_t& put_size_here, file_size_t max_size = 104857600);

		/// Same as other versions of get_contents(), but for std::string
		/// (no terminating 0 is needed inside the file, the string is 0-terminated anyway).
		/// \return false on failure (error is also set).
		inline bool get_contents(std::string& put_data_here, file_size_t max_size = 104857600);

		/// Write data to file, creating or truncating it beforehand.
		/// \c data may or may not be 0-terminated (it's irrelevant).
		/// \return false on failure (error is also set).
		inline bool put_contents(const unsigned char* data, file_size_t data_size);

		/// Same as the other version of put_contents(), but writes data from std::string.
		/// No terminating 0 is written to the file.
		/// \return false on failure (error is also set).
		inline bool put_contents(const std::string& data);


		/// Get file size. Do NOT assign the result to int - you'll break LFS support.
		/// If \c use_read is true, the file is read completely to determine its size.
		/// This is needed for special files (like in /proc), which have 0 size if queried
		/// the standard way.
		/// \return false on failure (error is also set).
		inline bool get_size(file_size_t& put_here, bool use_read = false);


		/// Move (rename) a file to \c to. The destination will be overwritten if it exists and
		/// it's not a directory (this is true even for win32, where renaming usually fails if
		/// the destination exists). This function is subject to rename() limitations.
		/// \return false on failure (error is also set).
		inline bool move(const std::string& to);

		/// Copy the file to a destination file specified by \c to.
		/// If \c to already exists, overwrite it.
		/// \return false on failure (error is also set).
		inline bool copy(const std::string& to);

		// remove() is in Path (parent).


		// --- Standard functions for portable implementation of operations
		// These are similar to their system equivalents. Additionally, they
		// use the best OS-dependent function if available.

		/// Same as std::fopen(), but platform-independent (properly handles charsets, etc...).
		static inline handle_type platform_fopen(const char* file, const char* open_mode);

		/// Same as fseek[o](), but platform-independent.
		static inline int platform_fseek(handle_type stream, file_size_t offset, int whence);

		/// Same as ftell[o](), but platform-independent.
		static inline file_size_t platform_ftell(handle_type stream);


	private:

		// for move semantics:

// 		File(const File& other);  // don't allow it. allow only from non-const.

// 		const File& operator=(const File& other);  // don't allow it. allow only from non-const.


		handle_type file_;  ///< File handle (FILE*)

};





// ------------------------------------------- Implementation


/*
// move semantics, as with copy constructor.
inline File& File::operator= (File& other)
{
	file_ = other.file_;

	// clear other's members, so everything is clear.
	other.file_ = NULL;
	other.set_path("");
	other.set_error(HZ__("The file handle ownership was transferred from this object."));

	return *this;
}
*/



inline bool File::open(const std::string& open_mode)
{
	clear_error();

	if (file_) {  // already open
		set_error(std::string(HZ__("Error while opening file \"/path1/\": "))
				+ HZ__("Another file is open already. Close it first."), 0, this->get_path());
		return false;
	}
	if (this->empty()) {
		set_error(std::string(HZ__("Error while opening file: ")) + HZ__("Supplied path is empty."));
		return false;
	}

	// this creates a 0 size file if it doesn't exist!
	file_ = platform_fopen(this->c_str(), open_mode.c_str());
	if (!file_) {
		set_error(HZ__("Error while opening file \"/path1/\": /errno/."), errno, this->get_path());
		return false;
	}

	return true;
}



inline bool File::close()
{
	clear_error();

	if (file_ && std::fclose(file_) != 0) {  // error
		set_error(HZ__("Error while closing file \"/path1/\": /errno/."), errno, this->get_path());
		file_ = NULL;
		return false;
	}

	file_ = NULL;
	return true;  // even if already closed
}



inline bool File::get_contents(unsigned char*& put_data_here,
		file_size_t& put_size_here, file_size_t max_size)
{
	return get_contents_noalloc(put_data_here, static_cast<file_size_t>(-1), put_size_here, max_size);
}



inline bool File::get_contents_noalloc(unsigned char*& put_data_here, file_size_t buf_size,
		file_size_t& put_size_here, file_size_t max_size)
{
	clear_error();

	if (this->empty()) {
		set_error(std::string(HZ__("Unable to open file for reading: ")) + HZ__("Supplied path is empty."));
		return false;
	}

	handle_type f = platform_fopen(this->c_str(), "rb");
	if (!f) {
		set_error(HZ__("Unable to open file \"/path1/\" for reading: /errno/."), errno, this->get_path());
		return false;
	}


	do {  // goto emulation

		if (platform_fseek(f, 0, SEEK_END) != 0) {
			set_error(HZ__("Unable to read file \"/path1/\": /errno/."), errno, this->get_path());
			break;  // goto cleanup
		}

		const file_size_t size = platform_ftell(f);

		if (size == static_cast<file_size_t>(-1)) {  // size may be unsigned, it's the way it works
			set_error(HZ__("Unable to read file \"/path1/\": /errno/."), errno, this->get_path());
			break;  // goto cleanup
		}

		if (size > max_size) {
			set_error(std::string(HZ__("Unable to read file \"/path1/\": ")) + HZ__("File size is larger than allowed."), 0, this->get_path());
			break;  // goto cleanup
		}

		// automatically allocate the buffer if buf_size is -1.
		bool auto_alloc = (buf_size == static_cast<file_size_t>(-1));
		if (!auto_alloc && buf_size < size) {
			set_error(std::string(HZ__("Unable to read file \"/path1/\": ")) + HZ__("Supplied buffer is too small."), 0, this->get_path());
			break;  // goto cleanup
		}

		std::rewind(f);  // returns void

		unsigned char* buf = put_data_here;
		if (auto_alloc)
			buf = new unsigned char[static_cast<unsigned int>(size)];  // this may throw!

		// We don't need large file support here because we read into memory,
		// which is limited at 31 bits anyway (on 32-bit systems).
		// I really hope this is not a byte-by-byte operation
		std::size_t read_bytes = std::fread(buf, 1, static_cast<std::size_t>(size), f);
		if (size != static_cast<file_size_t>(read_bytes)) {
			set_error(std::string(HZ__("Unable to read file \"/path1/\": "))
					+ HZ__("Unexpected number of bytes read."), 0, this->get_path());
			if (auto_alloc)
				delete[] buf;
			break;  // goto cleanup
		}

		// All OK
		put_size_here = size;
		if (auto_alloc)
			put_data_here = buf;

	} while (false);


	// cleanup:
	if (std::fclose(f) != 0) {
		if (!bad())  // don't overwrite the previous error
			set_error(HZ__("Error while closing file \"/path1/\": /errno/."), errno, this->get_path());
	}

	return ok();
}



inline bool File::get_contents(std::string& put_data_here, file_size_t max_size)
{
	file_size_t size = 0;
	unsigned char* buf = 0;
	get_contents(buf, size, max_size);
	if (bad())
		return false;

	// Note: No need for appending 0, it's all automatic in string.
	put_data_here.reserve(static_cast<std::string::size_type>(size));  // string takes size without trailing 0.
	put_data_here.append(reinterpret_cast<char*>(buf), static_cast<std::string::size_type>(size));

	delete[] buf;
	return ok();
}



inline bool File::put_contents(const unsigned char* data, file_size_t data_size)
{
	clear_error();

	if (this->empty()) {
		set_error(std::string(HZ__("Unable to open file for for writing: ")) + HZ__("Supplied path is empty."));
		return false;
	}

	handle_type f = platform_fopen(this->c_str(), "wb");
	if (!f) {
		set_error(HZ__("Unable to open file \"/path1/\" for writing: /errno/."), errno, this->get_path());
		return false;
	}

	// We write in chunks to support large files.
	const std::size_t chunk_size = 32*1024;  // 32K block devices will be happy.
	file_size_t left_to_write = data_size;

	bool write_error = false;
	while (left_to_write >= static_cast<file_size_t>(chunk_size)) {
		// better loop than specify all in one, this way we can support _really_ large files.
		if (std::fwrite(data + data_size - left_to_write, chunk_size, 1, f) != 1) {
			write_error = true;
			break;
		}
		left_to_write -= static_cast<file_size_t>(chunk_size);
	}

	// write the remainder
	if (!write_error && left_to_write > 0 && std::fwrite(data + data_size - left_to_write, static_cast<std::size_t>(left_to_write), 1, f) != 1)
		write_error = true;

	if (write_error) {
		set_error(std::string(HZ__("Unable to write file \"/path1/\": "))
				+ HZ__("Number of written bytes doesn't match the data size."), 0, this->get_path());
		std::fclose(f);  // don't check anything, it's too late
		return false;
	}

	if (std::fclose(f) != 0)
		set_error(HZ__("Error while closing file \"/path1/\": /errno/."), errno, this->get_path());

	return ok();
}



inline bool File::put_contents(const std::string& data)
{
	return put_contents(reinterpret_cast<const unsigned char*>(data.data()), static_cast<file_size_t>(data.size()));
}



inline bool File::get_size(file_size_t& put_here, bool use_read)
{
	clear_error();

	if (this->empty()) {
		set_error(std::string(HZ__("Unable to get file size: ")) + HZ__("Supplied path is empty."));
		return false;
	}

	if (!use_read) {
		// we could use _filelengthi64() for windows, but it doesn't seem to have
		// enough error reporting.

#if defined HAVE_WIN_LFS_FUNCS && HAVE_WIN_LFS_FUNCS
		struct _stati64 s;  // this contains 64-bit file size
		const int stat_result = _wstati64(this->get_utf16(), &s);
#elif defined _WIN32
		// The _stat* family may return stale info. _fstat*() doesn't have this problem
		// but requires opening the file, which may be bad (no permissions, etc...).
		struct _stat s;
		const int stat_result = _wstat(this->get_utf16(), &s);
#else
		struct stat s;
		const int stat_result = stat(this->c_str(), &s);
#endif
		if (stat_result == -1) {
			set_error(HZ__("Unable to get file size of \"/path1/\": /errno/."), errno, this->get_path());
			return false;
		}

#ifdef _WIN32
		if (!(s.st_mode & _S_IFREG) || (s.st_mode & _S_IFCHR))  // see FsPath::is_regular() for remarks.
#else
		if (!S_ISREG(s.st_mode))  // no need for S_ISLNK - we're using stat(), not lstat().
#endif
		{
			set_error("Unable to get file size of \"/path1/\": Supplied path is not a regular file.", 0, this->get_path());
			return false;
		}

		// For symlinks st_size is a size in bytes of the pointed file.
		// To get the size of a symlink itself, use lstat(). Size of a symlink
		// is basically strlen(pointed_path).
		put_here = s.st_size;

		return true;
	}


	// Force reading the file, assume it's non-seekable.

	handle_type f = platform_fopen(this->c_str(), "rb");
	if (!f) {
		set_error(HZ__("Unable to open file \"/path1/\" for reading: /errno/."), errno, this->get_path());
		return false;
	}

	const std::size_t buf_size = 32*1024;  // 32K block devices will be happy
	char buf[buf_size];

	// read until the end
	while (std::fread(buf, buf_size, 1, f) == 1) {
		// nothing
	}

	if (std::ferror(f)) {
		set_error(HZ__("Unable to read file \"/path1/\": /errno/."), errno, this->get_path());

	} else {  // file read completely

		const file_size_t size = platform_ftell(f);

		if (size == static_cast<file_size_t>(-1)) {  // size may be unsigned, it's the way it works
			set_error(HZ__("Unable to read file \"/path1/\": /errno/."), errno, this->get_path());

		} else {
			// All OK
			put_here = size;
		}
	}

	// cleanup:
	if (std::fclose(f) != 0) {
		if (!bad())  // don't overwrite the previous error
			set_error(HZ__("Error while closing file \"/path1/\": /errno/."), errno, this->get_path());
	}

	return ok();
}



inline bool File::move(const std::string& to)
{
	clear_error();

	if (this->empty()) {
		set_error(std::string(HZ__("Unable to move filesystem entry: ")) + HZ__("Source path is empty."));
		return false;
	}
	if (to.empty()) {
		set_error(std::string(HZ__("Unable to move filesystem entry \"/path1/\": "))
				+ HZ__("Destination path is empty."), 0, this->get_path());
		return false;
	}
	if (this->get_path() == to) {  // this is not bulletproof
		set_error(std::string(HZ__("Unable to move filesystem entry \"/path1/\": "))
				+ HZ__("Source path is the same as destination path."), 0, this->get_path());
		return false;
	}

	bool success = false;

#ifdef _WIN32
	success = (_wrename(this->get_utf16(), FsPath(to).get_utf16()) == 0);

	// win32 rename() doesn't replace contents if newpath exists and says "file exists".
	// Try to rename first, then unlink/rename again. This way we have at least some atomicity.
	if (!success && errno == EACCES) {
		File dest_file(to);
		if (dest_file.is_file()) {
			dest_file.remove();  // no error tracking here, rename() will report the needed error.
			success = (_wrename(this->get_utf16(), FsPath(to).get_utf16()) == 0);
		}
	}
#else
	success = (std::rename(this->c_str(), to.c_str()) == 0);

#endif

	if (!success)
		set_error(HZ__("Unable to move filesystem entry \"/path1/\" to \"/path2/\": /errno/."), errno, this->get_path(), to);

	return ok();
}




inline bool File::copy(const std::string& to)
{
	clear_error();

	if (this->empty()) {
		set_error(std::string(HZ__("Unable to copy file: ")) + HZ__("Source path is empty."));
		return false;
	}
	if (to.empty()) {
		set_error(std::string(HZ__("Unable to copy file \"/path1/\": "))
				+ HZ__("Destination path is empty."), 0, this->get_path());
		return false;
	}
	if (this->get_path() == to) {
		set_error(std::string(HZ__("Unable to copy file \"/path1/\": "))
				+ HZ__("Source path is the same as destination path."), 0, this->get_path());
		return false;
	}

	handle_type fsrc = platform_fopen(this->c_str(), "rb");
	if (!fsrc) {
		set_error(std::string(HZ__("Unable to copy file \"/path1/\" to \"/path2/\": "))
				+ HZ__("Unable to open source file: /errno/."), errno, this->get_path(), to);
		return false;
	}

	// Remember permissions in case the file is deleted while copying.
#if defined HAVE_WIN_LFS_FUNCS && HAVE_WIN_LFS_FUNCS
	struct _stati64 st;  // this contains 64-bit file size
	const int stat_result = _fstati64(_fileno(fsrc), &st);
#elif defined _WIN32
	struct _stat st;
	const int stat_result = _fstat(_fileno(fsrc), &st);  // win32 deprecated fstat, so use this instead.
#else
	struct stat st;
	const int stat_result = fstat(fileno(fsrc), &st);
#endif

	handle_type fdest = platform_fopen(to.c_str(), "wb");  // this truncates it
	if (!fdest) {
		std::fclose(fsrc);  // don't check status - there's nothing we can do anyway
		set_error(std::string(HZ__("Unable to copy file \"/path1/\" to \"/path2/\": "))
				+ HZ__("Unable to create destination file: /errno/."), errno, this->get_path(), to);
		return false;
	}


	const std::size_t buf_size = 32*1024;  // 32K
	unsigned char buf[buf_size] = {0};

	while (true) {
		// let's hope these are not byte-by-byte operations
		std::size_t read_bytes = std::fread(buf, 1, buf_size, fsrc);

		if (read_bytes != buf_size && std::ferror(fsrc)) {  // error
			set_error(std::string(HZ__("Unable to copy file \"/path1/\" to \"/path2/\": "))
					+ HZ__("Error while reading source file: /errno/."), errno, this->get_path(), to);
			break;
		}

		std::size_t written_bytes = std::fwrite(buf, 1, read_bytes, fdest);
		if (read_bytes != written_bytes) {  // error
			set_error(std::string(HZ__("Unable to copy file \"/path1/\" to \"/path2/\": "))
					+ HZ__("Error while writing to destination file: /errno/."), errno, this->get_path(), to);
			break;
		}
	}

	if (bad()) {  // error was set
		// don't check these - there's nothing we can do about them
		std::fclose(fsrc);
		std::fclose(fdest);
#ifdef _WIN32
		_wunlink(FsPath(to).get_utf16());
#else
		unlink(to.c_str());
#endif

		return false;
	}

	if (std::fclose(fsrc) == -1) {
		std::fclose(fdest);
#ifdef _WIN32
		_wunlink(FsPath(to).get_utf16());
#else
		unlink(to.c_str());
#endif

		set_error(std::string(HZ__("Unable to copy file \"/path1/\" to \"/path2/\": "))
				+ HZ__("Error while closing source file: /errno/."), errno, this->get_path(), to);
		return false;
	}

	if (std::fclose(fdest) == -1) {  // the OS may delay writing until this point (or even further).
#ifdef _WIN32
		_wunlink(FsPath(to).get_utf16());
#else
		unlink(to.c_str());
#endif

		set_error(std::string(HZ__("Unable to copy file \"/path1/\" to \"/path2/\": "))
				+ HZ__("Error while closing source file: /errno/."), errno, this->get_path(), to);
		return false;
	}

	// copy permissions. don't check for errors here - they are harmless.
	if (stat_result == 0) {
#ifdef _WIN32
		_wchmod(FsPath(to).get_utf16(), st.st_mode & (_S_IREAD | _S_IWRITE));  // it won't accept anything else.
#else
		chmod(to.c_str(), st.st_mode & 07777);  // don't transfer unneeded stuff (like "is directory").
#endif
	}

	return true;
}



File::handle_type inline File::platform_fopen(const char* file, const char* open_mode)
{
	// Don't validate parameters, they will be validated by the called functions.
#if defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
	handle_type f = 0;
	errno = _wfopen_s(&f, hz::scoped_array<wchar_t>(hz::win32_utf8_to_utf16(file)).get(),
			hz::scoped_array<wchar_t>(hz::win32_utf8_to_utf16(open_mode)).get() );
	return f;
#elif defined _WIN32
	return _wfopen(hz::scoped_array<wchar_t>(hz::win32_utf8_to_utf16(file)).get(),
			hz::scoped_array<wchar_t>(hz::win32_utf8_to_utf16(open_mode)).get());
#else
	return std::fopen(file, open_mode);
#endif
}



int inline File::platform_fseek(handle_type stream, file_size_t offset, int whence)
{
	// Don't validate parameters, they will be validated by the called functions.
#if defined HAVE_POSIX_OFF_T_FUNCS && HAVE_POSIX_OFF_T_FUNCS
	return fseeko(stream, offset, whence);  // POSIX
#elif defined HAVE_WIN_LFS_FUNCS && HAVE_WIN_LFS_FUNCS
	return _fseeki64(stream, offset, whence);
#else
	return std::fseek(stream, offset, whence);
#endif
}



file_size_t inline File::platform_ftell(handle_type stream)
{
#if defined HAVE_POSIX_OFF_T_FUNCS && HAVE_POSIX_OFF_T_FUNCS
	return ftello(stream);  // POSIX
#elif defined HAVE_WIN_LFS_FUNCS && HAVE_WIN_LFS_FUNCS
	return _ftelli64(stream);
#else
	return std::ftell(stream);
#endif
}






}  // ns hz



#endif

/// @}
