/**************************************************************************
 Copyright:
      (C) 2009 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_FS_DIR_PLATFORM_H
#define HZ_FS_DIR_PLATFORM_H

#include "hz_config.h"  // feature macros

#include <string>

#ifdef _WIN32
	#include <new> // std::nothrow
	#include <io.h>  // _* :)
	#include <cstddef>  // std::size_t
	#include <cstring>  // for string.h
	#include <string.h>  // wcslen()
#else
	#include <sys/types.h>  // dirent needs this
	#include <dirent.h>
#endif

#ifdef _WIN32
	#include "win32_tools.h"  // hz::win32_utf8_to_utf16(), hz::win32_utf16_to_utf8
	#include "scoped_array.h"  // hz::scoped_array
#endif

// Limited dirent-like API to hide platform differences.

// This API accepts/gives utf-8 filenames/paths on win32,
// current locale filenames/paths on others (just like glib).


namespace hz {



#ifndef _WIN32


typedef DIR* directory_handle_type;
typedef struct dirent* directory_entry_handle_type;

// Linux man pages say off_t, bit POSIX and glibc have long.
// typedef off_t directory_offset_type;
typedef long directory_offset_type;


inline directory_handle_type directory_open(const char* path)  // NULL on error, also sets errno.
{
	return opendir(path);
}

inline int directory_close(directory_handle_type dir)  // 0 on success, -1 on error (also sets errno).
{
	return closedir(dir);
}

inline void directory_rewind(directory_handle_type dir)
{
	return rewinddir(dir);
}

inline directory_entry_handle_type directory_read(directory_handle_type dir)  // sets errno and returns NULL on error
{
	return readdir(dir);
}

inline directory_offset_type directory_tell(directory_handle_type dir)
{
	return telldir(dir);
}

// return to the position given by directory_tell().
inline void directory_seek(directory_handle_type dir, directory_offset_type pos)
{
	seekdir(dir, pos);
}

inline std::string directory_entry_name(directory_entry_handle_type entry)
{
	return (entry && entry->d_name) ? entry->d_name : "";
}



#else  // win32:


// Note: Windows implementation is based on MinGW's dirent implementation
// (public domain).


namespace internal {

	struct DirectoryEntry {
		const wchar_t* d_name;  // this is owned by Directory's data field. utf-16.
	};

	struct Directory {
		struct _wfinddata_t data;  // entry data as given by _wfindfirst().
		DirectoryEntry entry;  // current directory entry
		intptr_t handle;  // directory handle as returned by _wfindfirst().
		long status;  // 0 - not started, -1 - off the end, positive - 0-based index of next entry.
		wchar_t full_pattern[MAX_PATH + 3];  // directory search pattern + "\\*"
	};

}


typedef internal::Directory* directory_handle_type;
typedef internal::DirectoryEntry* directory_entry_handle_type;
typedef long directory_offset_type;  // that's what they use...



inline directory_handle_type directory_open(const char* path)  // NULL on error, also sets errno.
{
	errno = 0;

	if (!path) {
		errno = ENOENT;  // not specified by opendir (3), not sure about this.
		return 0;
	}
	if (path[0] == '\0') {
		errno = ENOENT;  //   // Directory does not exist, or name is an empty string.
		return 0;
	}

	hz::scoped_array<wchar_t> wpath(hz::win32_utf8_to_utf16(path));
	if (!wpath) {
		errno = ENOENT;  // not specified by opendir (3), not sure about this.
		return 0;
	}

	struct _stat s;
	if (_wstat(wpath.get(), &s) == -1) {
		errno = ENOENT;  // Directory does not exist, or name is an empty string.
		return 0;
	}
	if (!(s.st_mode & _S_IFDIR)) {
		errno = ENOTDIR;  // Name is not a directory.
		return 0;
	}

	internal::Directory* dir = new(std::nothrow) internal::Directory();
	if (!dir) {
		errno = ENOMEM;  // Out of memory
		return 0;
	}

	// We store an absolute path so that we don't depend on current directory
	// (after rewind, for example).
	_wfullpath(dir->full_pattern, wpath.get(), MAX_PATH);  // make an absolute path
	std::size_t len = wcslen(dir->full_pattern);

	if (len == 0 || (len+1) >= MAX_PATH) {  // not sure about the conditions, but just in case.
		errno = ENOTDIR;  //   // Directory does not exist, or name is an empty string.
		return 0;
	}

	// ms says that it can be both slash and backslash
	if (dir->full_pattern[len - 1] != '/' && dir->full_pattern[len - 1] != '\\') {
		dir->full_pattern[len] = L'\\';
		dir->full_pattern[len+1] = L'*';
	} else {
		dir->full_pattern[len] = L'*';
	}

	dir->handle = -1;  // not actually opened yet
	dir->status = 0;  // not started yet

	return dir;
}



inline int directory_close(directory_handle_type dir)  // 0 on success, -1 on error (also sets errno).
{
	errno = 0;

	if (!dir) {
		errno = EBADF;  // Invalid directory stream descriptor
		return -1;
	}

	int result = 0;

	if (dir->handle != -1) {
		result = _findclose(dir->handle);  // -1 on error, 0 on success.
		// It sets ENOENT (no matching entries found) on error, which doesn't
		// really make sense.
	}

	delete dir;

	return result;
}



inline void directory_rewind(directory_handle_type dir)
{
	// rewinddir (3p) defines no errors.
// 	errno = 0;

	if (!dir) {
// 		errno = EFAULT;
		return;
	}

	if (dir->handle != -1) {
		_findclose(dir->handle);
	}

	dir->handle = -1;
	dir->status = 0;
}



inline directory_entry_handle_type directory_read(directory_handle_type dir)
{
	errno = 0;

	if (!dir) {
		errno = EBADF;  // Invalid directory stream descriptor.
		return 0;
	}

	if (dir->status < 0) 	{  // we already reached the end
		return 0;
	}

	if (dir->status == 0) {  // haven't started yet
		dir->handle = _wfindfirst(dir->full_pattern, &(dir->data));
		if (dir->handle == -1) {  // no files in directory
			dir->status = -1;
			if (errno == ENOENT) {  // no entries, it's not an error in readdir().
				errno = 0;
			}
			return 0;
		}
		dir->status = 1;

	} else {  // already started, get the next entry
		if (_wfindnext(dir->handle, &(dir->data))) {  // error or no more files
			if (errno == ENOENT) {
				errno = 0;
			}
			_findclose(dir->handle);
			dir->handle = -1;
			dir->status = -1;

		} else {  // next entry read ok
			++(dir->status);
		}
	}

	if (dir->status > 0) {
		dir->entry.d_name = (dir->data.name ? dir->data.name : L"");
		return &(dir->entry);
	}

	return 0;
}



inline directory_offset_type directory_tell(directory_handle_type dir)
{
	errno = 0;  // POSIX doesn't specify errno on telldir(), Linux does.

	if (!dir || dir->status == -1) {
		errno = EBADF;  // Invalid directory stream descriptor
		return -1;
	}
	return dir->status;  // just the position
}



// return to the position given by directory_tell().
inline void directory_seek(directory_handle_type dir, directory_offset_type pos)
{
	// seekdir (3p) defines no errors.
// 	errno = 0;

	if (!dir) {
// 		errno = EFAULT;
		return;
	}

	if (pos < -1) {  // invalid position
// 		errno = EINVAL;
		return;
	}

	if (pos == -1) {  // past the end
		if (dir->handle != -1) {
			_findclose (dir->handle);  // close it like directory_read does.
		}
		dir->handle = -1;
		dir->status = -1;

	} else {  // ok, rewind and read in a loop
		directory_rewind(dir);

		while (dir->status < pos) {
			if (!directory_read(dir))
				break;
		}
	}
}



inline std::string directory_entry_name(directory_entry_handle_type entry)
{
	if (!entry->d_name)
		return std::string();

	return win32_utf16_to_utf8_string(entry->d_name);
}



#endif  // win32


}  // ns hz



#endif
