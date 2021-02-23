/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
License: Zlib
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_FS_H
#define HZ_FS_H

#include <string>
#include <string_view>
#include <cerrno>
#include <system_error>
#include <cstdio>  // std::FILE, std::fopen() and friends
#include <stdio.h>  // off_t, fileno(), _fileno(), _wfopen()
#include <limits>

#ifdef _WIN32
	#include <io.h>  // _waccess*()
#else
	#include <cstddef>  // std::size_t
	#include <unistd.h>  // access()
#endif

#if defined __MINGW32__
	#include <_mingw.h>  // MINGW_HAS_SECURE_API
#endif

#include "env_tools.h"  // hz::env_get_value

#ifdef _WIN32
	#include "win32_tools.h"  // win32_* stuff
#endif

#include "fs_ns.h"


/// \def HAVE_POSIX_OFF_T_FUNCS
/// Defined to 0 or 1. If 1, compiler supports fseeko/ftello. See fs_file.h for details.
#ifndef HAVE_POSIX_OFF_T_FUNCS
	// It's quite hard to detect, so we just enable for any non-win32.
	#ifndef _WIN32
		#define HAVE_POSIX_OFF_T_FUNCS 1
	#endif
#endif

/// \def HAVE_WIN_LFS_FUNCS
/// Defined to 0 or 1. If 1, compiler supports LFS (large file support) functions on win32.
/// It seems that LFS functions are always defined in mingw-w64 and msvc >= 2005.
#ifndef HAVE_WIN_LFS_FUNCS
	#ifdef _WIN32
		#define HAVE_WIN_LFS_FUNCS 1
	#endif
#endif


/**
\file
Filesystem utilities
*/


namespace hz {



#ifdef _WIN32
	// Unlike std::filesystem::path::preferred_separator, this is always char.
	constexpr char fs_preferred_separator = '\\';
#else
	constexpr char fs_preferred_separator = '/';
#endif



/// \typedef platform_file_size_t
/// Offset & size type. may be uint32_t or uint64_t, depending on system and compilation flags.
/// Note: It is usually discouraged to use this type in headers, because the library may be
/// compiled with one size and the application with another. However, this problem is rather
/// limited if using header-only approach, like we do.

#if defined HAVE_POSIX_OFF_T_FUNCS && HAVE_POSIX_OFF_T_FUNCS
	using platform_file_size_t = off_t;  // off_t is in stdio.h, available in all self-respecting unix systems.
#elif defined HAVE_WIN_LFS_FUNCS && HAVE_WIN_LFS_FUNCS
	using platform_file_size_t = long long;  // ms stuff, _fseeki64() and friends use it.
#else
	using platform_file_size_t = long;  // fseek() and friends use long. win32 doesn't have off_t.
#endif



/// Platform-dependent fopen(). Sets errno on error.
inline std::FILE* fs_platform_fopen(const fs::path& file, const char* open_mode)
{
	// Don't validate parameters, they will be validated by the called functions.
#if defined MINGW_HAS_SECURE_API || defined _MSC_VER
	std::FILE* f = nullptr;
	errno = _wfopen_s(&f, file.c_str(), hz::win32_utf8_to_utf16(open_mode).c_str());
	return f;
#elif defined _WIN32
	return _wfopen(file.c_str(), hz::win32_utf8_to_utf16(open_mode).c_str());
#else
	return std::fopen(file.c_str(), open_mode);
#endif
}



/// Platform-dependent fseek(). Sets errno on error.
inline int fs_platform_fseek(FILE* stream, std::uintmax_t offset, int whence)
{
	// Don't validate parameters, they will be validated by the called functions.
#if defined HAVE_POSIX_OFF_T_FUNCS && HAVE_POSIX_OFF_T_FUNCS
	return fseeko(stream, (off_t)offset, whence);  // POSIX
#elif defined HAVE_WIN_LFS_FUNCS && HAVE_WIN_LFS_FUNCS
	return _fseeki64(stream, (long long)offset, whence);
#else
	return std::fseek(stream, (long)offset, whence);
#endif
}



/// Platform-dependent ftell(). Sets errno on error.
inline std::uintmax_t fs_platform_ftell(FILE* stream)
{
	platform_file_size_t pos = 0;
#if defined HAVE_POSIX_OFF_T_FUNCS && HAVE_POSIX_OFF_T_FUNCS
	pos = ftello(stream);  // POSIX
#elif defined HAVE_WIN_LFS_FUNCS && HAVE_WIN_LFS_FUNCS
	pos = _ftelli64(stream);
#else
	pos = std::ftell(stream);
#endif
	if (pos == static_cast<platform_file_size_t>(-1)) {
		return static_cast<std::uintmax_t>(-1);
	}
	return static_cast<std::uintmax_t>(pos);
}



/// Same as other versions of get_contents(), but puts data into an already
/// allocated buffer of size \c buf_size.
/// If the size is insufficient, false is returned and buffer is left untouched.
/// If any other error occurs, the buffer is left in unspecified state.
/// Internal usage only: If buf_size is -1, the buffer will be automatically allocated.
/// TODO: Support files which are not seekable and don't have a size attribute (e.g. /proc/*).
/// \return Empty (zero) error code on success.
inline std::error_code fs_file_get_contents_noalloc(const fs::path& file, unsigned char*& put_data_here, std::uintmax_t buf_size,
		std::uintmax_t& put_size_here, std::uintmax_t max_size)
{
	if (file.empty()) {
		return std::make_error_code(std::errc::invalid_argument);
	}

	std::FILE* f = fs_platform_fopen(file, "rb");
	if (!f) {
		return std::error_code(errno, std::system_category());
	}

	std::error_code ec;

	do {  // goto emulation

		if (fs_platform_fseek(f, 0, SEEK_END) != 0) {
			ec = std::error_code(errno, std::system_category());
			break;  // goto cleanup
		}

		// We can't use fs::file_size() here since it will introduce a race condition.
		const std::uintmax_t size = fs_platform_ftell(f);

		if (size == static_cast<std::uintmax_t>(-1)) {  // size may be unsigned, it's the way it works
			ec = std::error_code(errno, std::system_category());
			break;  // goto cleanup
		}

		if (size > max_size) {
			ec = std::make_error_code(std::errc::file_too_large);
			break;  // goto cleanup
		}

		// automatically allocate the buffer if buf_size is -1.
		bool auto_alloc = (buf_size == static_cast<std::uintmax_t>(-1));
		if (!auto_alloc && buf_size < size) {
			ec = std::make_error_code(std::errc::no_buffer_space);
			break;  // goto cleanup
		}

		std::rewind(f);  // returns void

		unsigned char* buf = put_data_here;
		if (auto_alloc)
			buf = new unsigned char[static_cast<std::size_t>(size)];  // this may throw!

		// We don't need large file support here because we read into memory,
		// which is limited at 31 bits anyway (on 32-bit systems).
		// I really hope this is not a byte-by-byte operation
		std::size_t read_bytes = std::fread(buf, 1, static_cast<std::size_t>(size), f);
		if (size != static_cast<std::uintmax_t>(read_bytes)) {
			// Unexpected number of bytes read. Not sure if this is reflected in errno or not.
			ec = std::error_code(errno, std::system_category());
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
		if (!ec) {  // don't overwrite the previous error
			ec = std::error_code(errno, std::system_category());
		}
	}

	return ec;
}



/// Get file contents. \c put_data_here will be allocated to whatever
/// size is needed to contain all the data. The size is written to \c put_size_here.
/// You must call "delete[] put_data_here" afterwards.
/// If the file is larger than \c max_size (100M by default), the function refuses to load it.
/// Note: No additional trailing 0 is written to data!
/// \return Empty (zero) error code on success.
inline std::error_code fs_file_get_contents(const fs::path& file, unsigned char*& put_data_here,
		std::uintmax_t& put_size_here, std::uintmax_t max_size)
{
	return fs_file_get_contents_noalloc(file, put_data_here, static_cast<std::uintmax_t>(-1), put_size_here, max_size);
}



/// Same as other versions of get_contents(), but for std::string
/// (no terminating 0 is needed inside the file, the string is 0-terminated anyway).
/// \return Empty (zero) error code on success.
inline std::error_code fs_file_get_contents(const fs::path& file, std::string& put_data_here, std::uintmax_t max_size)
{
	std::uintmax_t size = 0;
	unsigned char* buf = nullptr;
	std::error_code ec = fs_file_get_contents(file, buf, size, max_size);
	if (ec)
		return ec;

	// Note: No need for appending 0, it's all automatic in string.
	put_data_here.reserve(static_cast<std::string::size_type>(size));  // string takes size without trailing 0.
	put_data_here.append(reinterpret_cast<char*>(buf), static_cast<std::string::size_type>(size));

	delete[] buf;
	return ec;
}



/// Procfs files don't support SEEK_END or ftello(). They can't
/// be read using fs_file_get_contents, so use this function instead.
inline std::error_code fs_file_get_contents_unseekable(const hz::fs::path& file, std::string& put_data_here)
{
	std::FILE* fp = fs_platform_fopen(file, "rb");
	if (!fp) {
		return std::error_code(errno, std::system_category());;
	}
	put_data_here.clear();

	char line[1024] = {0};
	while (std::fgets(line, static_cast<int>(sizeof(line)), fp) != nullptr) {
		if (*line != '\0')
			put_data_here += line;  // line contains the terminating newline as well
	}

	std::fclose(fp);

	return std::error_code();
}



/// Write data to file, creating or truncating it beforehand.
/// \c data may or may not be 0-terminated (it's irrelevant).
/// \return Empty (zero) error code on success.
inline std::error_code fs_file_put_contents(const fs::path& file, const unsigned char* data, std::uintmax_t data_size)
{
	if (file.empty()) {
		return std::make_error_code(std::errc::invalid_argument);
	}

	std::FILE* f = fs_platform_fopen(file, "wb");
	if (!f) {
		return std::error_code(errno, std::system_category());
	}

	// We write in chunks to support large files.
	const std::size_t chunk_size = 32*1024;  // 32K block devices will be happy.
	std::uintmax_t left_to_write = data_size;

	bool write_error = false;
	while (left_to_write >= static_cast<std::uintmax_t>(chunk_size)) {
		// better loop than specify all in one, this way we can support _really_ large files.
		if (std::fwrite(data + data_size - left_to_write, chunk_size, 1, f) != 1) {
			write_error = true;
			break;
		}
		left_to_write -= static_cast<std::uintmax_t>(chunk_size);
	}

	// write the remainder
	if (!write_error && left_to_write > 0 && std::fwrite(data + data_size - left_to_write, static_cast<std::size_t>(left_to_write), 1, f) != 1)
		write_error = true;

	if (write_error) {
		// Number of written bytes doesn't match the data size. Not sure if this is reflected in errno or not.
		auto ec = std::error_code(errno, std::system_category());
		std::fclose(f);  // don't check anything, it's too late
		return ec;
	}

	if (std::fclose(f) != 0)
		return std::error_code(errno, std::system_category());

	return std::error_code();
}



/// Same as the other version of put_contents(), but writes data from std::string.
/// No terminating 0 is written to the file.
/// \return Empty (zero) error code on success.
inline std::error_code fs_file_put_contents(const fs::path& file, const std::string_view& data)
{
	return fs_file_put_contents(file, reinterpret_cast<const unsigned char*>(data.data()), static_cast<std::uintmax_t>(data.size()));
}





/// Get the current user's home directory.
/// This function always returns something, but
/// note that the directory may not actually exist at all.
inline fs::path fs_get_home_dir()
{
	// Do NOT use g_get_home_dir, it doesn't work consistently in win32
	// between glib versions.

#ifdef _WIN32
	// For windows we usually get "C:\documents and settings\username".
	// Try $USERPROFILE, then CSIDL_PROFILE, then Windows directory
	// (glib uses it, not sure why though).

	std::string dir;
	hz::env_get_value("USERPROFILE", dir);  // in utf-8

	if (dir.empty()) {
		dir = win32_get_special_folder(CSIDL_PROFILE);
	}
	if (dir.empty()) {
		dir = win32_get_windows_directory();  // always returns something.
	}

	return fs::u8path(dir);

#else  // linux, etc...
	// We use $HOME to allow the user to override it.
	// Other solutions involve getpwuid_r() to read from passwd.
	std::string dir;
	if (!hz::env_get_value("HOME", dir)) {  // works well enough
		// HOME may be empty in some situations (limited shells
		// and rescue logins).
		// We could use /tmp/<username>, but obtaining the username
		// is too complicated and unportable.

		std::error_code ec;
		return fs::temp_directory_path(ec);
	}

	return fs::path(dir);  // native encoding
#endif
}



/// Get the current user's configuration file directory (in native fs encoding
/// for UNIX, utf-8 for windows). E.g. "$HOME/.config" in UNIX.
inline fs::path fs_get_user_config_dir()
{
	fs::path path;

#ifdef _WIN32
	// that's "C:\documents and settings\username\application data".
	path = fs::u8path(win32_get_special_folder(CSIDL_APPDATA));
	if (path.empty()) {
		path = fs_get_home_dir();  // fallback, always non-empty.
	}
#else
	std::string dir;
	if (hz::env_get_value("XDG_CONFIG_HOME", dir)) {
		path = dir;  // native encoding.
	} else {
		// default to $HOME/.config
		path = fs_get_home_dir() / ".config";
	}
#endif
	return path;
}



/// Check if the existing file can be fopen'ed with "rb", or the directory has read perms.
/// Note: This function should be use only as an utility function (e.g. for GUI notification);
/// other uses are not logically concurrent-safe (and therefore, insecure).
/// ec is set to error code in case of failure.
inline bool fs_path_is_readable(const fs::path& path, std::error_code& ec)
{
	if (path.empty()) {
		ec = std::make_error_code(std::errc::invalid_argument);
		return false;
	}

#if defined MINGW_HAS_SECURE_API || defined _MSC_VER
	if (_waccess_s(path.c_str(), 04))  // msvc uses integers instead (R_OK == 04 anyway).
#elif defined _WIN32
	if (_waccess(path.c_str(), 04) == -1)
#else
	if (access(path.c_str(), R_OK) == -1)  // from unistd.h
#endif
	{
		ec = std::error_code(errno, std::system_category());
		return false;
	}
	return true;
}



/// Check if the existing or soon to be created file is writable, or if files can be created in this dir.
/// Note: The same security considerations apply to this function as to is_readable().
inline bool fs_path_is_writable(const fs::path& path, std::error_code& ec)
{
	if (path.empty()) {
		ec = std::make_error_code(std::errc::invalid_argument);
		return false;
	}

	std::error_code ignored_ec;  // ignore this error
	bool is_directory = fs::is_directory(path, ignored_ec);
	bool path_exists = fs::exists(path, ec);
	if (ec) {
		return false;
	}

	fs::path dirname = (is_directory ? path : path.parent_path());

#ifdef _WIN32  // win32 doesn't get access() (it just doesn't work with writing)

	// If it doesn't exist, try to create it.
	// If it exists and is a file, try to open it for writing.
	// If it exists and is a directory, try to create a test file in it.
	// Note: This method is possibly non-suitable for symlink-capable filesystems.

	fs::path path_to_check = path;
	if (path_exists && is_directory) {
		path_to_check /= "__test.txt";
		path_exists = fs::exists(path_to_check.exists(), ignored_ec);
	}

	// path_to_check either doesn't exist, or it's a file. try to open it.
	std::FILE* f = fs_platform_fopen(path_to_check, "ab");  // this creates a 0 size file if it doesn't exist!
	if (!f) {
		ec = std::error_code(errno, std::system_category());
		return false;
	}

	if (std::fclose(f) != 0) {
		ec = std::error_code(errno, std::system_category());
		return false;
	}

	// remove the created file
	if (path_exists && _wunlink(path_to_check.c_str()) == -1) {
		ec = std::error_code(errno, std::system_category());
		return false;
	}

	// All OK

#else

	if (path_exists && is_directory) {
		if (access(dirname.c_str(), W_OK) == -1) {
			ec = std::error_code(errno, std::system_category());
			return false;
		}

	} else {  // no such path or it's a file
		if (path_exists) {  // checking an existing file
			if (access(path.c_str(), W_OK) == -1) {
				ec = std::error_code(errno, std::system_category());
				return false;
			}

		} else {  // no such path, check parent dir's access mode
			if (access(dirname.c_str(), W_OK) == -1) {
				ec = std::error_code(errno, std::system_category());
				return false;
			}
		}
	}

#endif

	return true;
}




/// Change the supplied filename so that it's safe to create it
/// (remove any potentially harmful characters from it).
inline std::string fs_filename_make_safe(const std::string_view& filename)
{
	std::string s(filename);
	std::string::size_type pos = 0;
	while ((pos = s.find_first_not_of(
			"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890._-",
			pos)) != std::string::npos) {
		s[pos] = '_';
		++pos;
	}
	// win32 kernel (heh) has trouble with space and dot-ending files
	if (!s.empty() && (s[s.size() - 1] == '.' || s[s.size() - 1] == ' ')) {
		s[s.size() - 1] = '_';
	}
	return s;
}




}  // ns hz



#endif

/// @}
