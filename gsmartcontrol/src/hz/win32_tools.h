/**************************************************************************
 Copyright:
      (C) 2003 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_WIN32_TOOLS_H
#define HZ_WIN32_TOOLS_H

#include "hz_config.h"  // feature macros

#ifdef _WIN32  // Guards this header completely

#include <windows.h>  // lots of stuff
#include <shlobj.h>  // SH*
#include <io.h>  // _open_osfhandle
#include <fcntl.h>  // _O_TEXT

#include <string>
#include <cctype>  // for ctype.h
#include <ctype.h>  // towupper
#include <cstdio>  // for stdio.h, std::f*, std::setvbuf, std::fprintf, std::FILE
#include <stdio.h>  // _fdopen, _wfopen, _wremove
#include <cstdlib>  // std::atexit
#include <ios>  // std::ios::sync_with_stdio



/**
\file
\brief Win32-specific API functions.
\note The public API works with UTF-8 strings, unless noted otherwise.
*/



namespace hz {


/// Get a list of drives available for the user. The drives are in
/// "C:\" format (uppercase, utf-8).
template<class Container> inline
bool win32_get_drive_list(Container& put_here);


/// Get windows "special" folder by CSIDL (CSIDL_APPDATA, for example).
/// See http://msdn.microsoft.com/en-us/library/bb762494(VS.85).aspx
/// for details.
inline std::string win32_get_special_folder(int csidl, bool auto_create = true);


/// Usually C:\\Windows or something like that.
inline std::string win32_get_windows_directory();


/// Get registry value as a string.
/// Base may be e.g. HKEY_CURRENT_USER.
/// Note that this works only with REG_SZ (string) types,
/// returning their value as utf-8. False is returned for all other types.
inline bool win32_get_registry_value_string(HKEY base,
		const std::string& keydir, const std::string& key, std::string& put_here);


/// Set registry value as a string.
/// Note that this sets the type as REG_SZ (string). The accepted utf-8
/// value is converted to utf-16 for storage.
inline bool win32_set_registry_value_string(HKEY base,
		const std::string& keydir, const std::string& key, const std::string& value);


/// Redirect stdout and stderr to console window (if open). Requires winxp (at compile-time).
/// \param create_if_none if true, create a new console if none was found and attach to it.
/// \return false if failed or unsupported.
inline bool win32_redirect_stdio_to_console(bool create_if_none = false);


/// Redirect stdout and stderr to console window (if open). Requires winxp (at compile-time).
/// \param create_if_none if true, create a new console if none was found and attach to it.
/// \param console_created Set to true if a new console was created due to \c create_if_none.
/// \return false if failed or unsupported.
inline bool win32_redirect_stdio_to_console(bool create_if_none, bool& console_created);


/// Redirect stdout to stdout.txt and stderr to stderr.txt.
/// Call once.
/// \return true on success, false on failure.
inline bool win32_redirect_stdio_to_files(std::string stdout_file = "", std::string stderr_file = "");


/// Convert a string in user-specified encoding to utf-16 string (00-terminated array of wchar_t).
/// wchar_t is 2 bytes on windows, thus holding utf-16 code unit.
/// Caller must delete[] the returned value.
/// returned_buf_size (if not 0) will be set to the size of returned buffer
/// (in wchar_t; including terminating 00). The byte size will be (returned_buf_size*2).
inline wchar_t* win32_user_to_utf16(UINT from_cp, const char* from_str, unsigned int* returned_buf_size = 0);


/// Convert utf-16 string (represented as 0-terminated array of wchar_t, where
/// wchar_t is 2 bytes (on windows)) to a string in user-specified encoding.
/// Caller must delete[] the returned value.
/// The size of the returned buffer is std::strlen(buf) + 1.
/// returned_buf_size (if not 0) will be set to the size of returned buffer
/// (in char; including terminating 0).
inline char* win32_utf16_to_user(UINT to_cp, const wchar_t* utf16_str, unsigned int* returned_buf_size = 0);


/// Convert utf-8 string to utf-16 string. See win32_user_to_utf16() for details.
inline wchar_t* win32_utf8_to_utf16(const char* utf8_str, unsigned int* returned_buf_size = 0);


/// Convert utf-16 string to utf-8 string. See win32_utf16_to_user() for details.
inline char* win32_utf16_to_utf8(const wchar_t* utf16_str, unsigned int* returned_buf_size = 0);


/// Same as win32_utf16_to_utf8(), but safer since it never returns 0 (it returns "" instead).
/// Note that we don't provide non-utf8 versions of this function, because the other charsets
/// may actually be multi-byte (std::string can't hold their terminating 0 properly).
/// Also, we don't provide std::wstring versions for utf16, because they are not always
/// supported by non-MS compilers (mingw, for example).
inline std::string win32_utf16_to_utf8_string(const wchar_t* utf16_str);


/// Convert current locale-encoded string to utf-16 string. See win32_user_to_utf16() for details.
/// Use CP_ACP is system default windows ANSI codepage.
/// Use CP_THREAD_ACP is for current thread. Think before you choose. :)
inline wchar_t* win32_locale_to_utf16(const char* loc_str, unsigned int* returned_buf_size = 0,
		bool use_thread_locale = false);


/// Convert utf-16 string to locale-encoded string. See win32_utf16_to_user() for details.
inline char* win32_utf16_to_locale(const wchar_t* utf16_str, unsigned int* returned_buf_size = 0,
		bool use_thread_locale = false);


/// Convert current locale-encoded string to utf-8 string. The returned value must be freed by caller.
inline char* win32_locale_to_utf8(const char* loc_str, unsigned int* returned_buf_size = 0,
		bool use_thread_locale = false);


/// Convert utf-8 string to locale-encoded string. See win32_utf8_to_user() for details.
inline char* win32_utf8_to_locale(const char* utf8_str, unsigned int* returned_buf_size = 0,
		bool use_thread_locale = false);






// ------------------------------------------- Implementation




// Get a list of drives available for the user. The drives are in
// "C:\" format (uppercase, utf-8).
template<class Container> inline
bool win32_get_drive_list(Container& put_here)
{
	DWORD buf_size = GetLogicalDriveStringsW(0, 0);  // find out the required buffer size
	if (!buf_size)  // error
		return false;

	wchar_t* buf = new wchar_t[buf_size];

	// Returns a consecutive array of 00-terminated strings (each 00-terminated).
	if (!GetLogicalDriveStringsW(buf_size, buf)) {
		delete[] buf;
		return false;
	}

	wchar_t* text_ptr = buf;

	// add the drives to list
	while (*text_ptr != 0) {
		text_ptr[0] = static_cast<wchar_t>(towupper(text_ptr[0]));
		std::string drive = win32_utf16_to_utf8_string(text_ptr);
		if (!drive.empty()) {
			put_here.push_back(drive + ":\\");
		}
		while (*text_ptr != 0)
			++text_ptr;
		++text_ptr;  // set to position after 0
	}

	delete[] buf;

	return true;
}



// Get windows "special" folder by CSIDL (CSIDL_APPDATA, for example).
// See http://msdn.microsoft.com/en-us/library/bb762494(VS.85).aspx
// for details.
inline std::string win32_get_special_folder(int csidl, bool auto_create)
{
	wchar_t buf[MAX_PATH] = {0};

	// SHGetSpecialFolderPath() is since ie4
#if (defined _WIN32_IE) && (_WIN32_IE >= 0x0400)
	if (SHGetSpecialFolderPathW(NULL, buf, csidl, auto_create)) {
		return win32_utf16_to_utf8_string(buf);
	}

#else
	LPITEMIDLIST pidl = 0;

	if (SHGetSpecialFolderLocation(NULL, csidl, &pidl) == S_OK) {
		if (SHGetPathFromIDListW(pidl, buf)) {
			errno = 0;
			// 00 is F_OK (for msvc). ENOENT is "no such file or directory".
			// We can't really do anything if errors happen here, so ignore them.
			if (auto_create && _waccess(buf, 00) == -1 && errno == ENOENT) {
				_wmkdir(buf);
			}
			return win32_utf16_to_utf8_string(buf);
		}

		// We must free pidl. This can be done in two ways:

		// CoTaskMemFree() requires linking to ole32.
		// CoTaskMemFree(pidl);

		// SHGetMalloc() which is deprecated and may be removed in the future.
		IMalloc* allocator = 0;
		if (SHGetMalloc(&allocator) == S_OK && allocator) {
			allocator->Free(pidl);
			allocator->Release();
		}
	}

#endif

	return std::string();
}



// Usually C:\Windows or something like that.
inline std::string win32_get_windows_directory()
{
	wchar_t buf[MAX_PATH];
	GetWindowsDirectoryW(buf, MAX_PATH);
	return (buf[0] ? win32_utf16_to_utf8_string(buf) : "C:\\");
}



// base may be e.g. HKEY_CURRENT_USER.
// Note that this works only with REG_SZ (string) types,
// returning their value as utf-8. False is returned for all other types.
inline bool win32_get_registry_value_string(HKEY base,
		const std::string& keydir, const std::string& key, std::string& put_here)
{
	wchar_t* wkeydir = win32_utf8_to_utf16(keydir.c_str());
	if (!wkeydir)
		return false;

	HKEY reg_key = NULL;
	bool open_status = (RegOpenKeyExW(base, wkeydir, 0, KEY_QUERY_VALUE, &reg_key) == ERROR_SUCCESS);

	delete[] wkeydir;

	if (!open_status)
		return false;

	wchar_t* wkey = win32_utf8_to_utf16(key.c_str());
	if (!wkey) {  // conversion error. Note that an empty string is not an error.
		if (reg_key)
			RegCloseKey(reg_key);
		return false;
	}

	DWORD type = 0;
	DWORD nbytes = 0;
	// This returns the size, including the 0 character only if the data was stored with it.
	bool status = (RegQueryValueExW(reg_key, wkey, 0, &type, NULL, &nbytes) == ERROR_SUCCESS);

	if (status) {
		DWORD buf_len = (nbytes % 2 ? (nbytes+1) : nbytes);  // 00-terminate on 2-byte boundary, in case the value isn't.
		BYTE* result = new BYTE[buf_len];
		result[buf_len-1] = result[buf_len-2] = result[buf_len-3] = 0;

		status = (RegQueryValueExW(reg_key, wkey, 0, &type, result, &nbytes) == ERROR_SUCCESS);

		if (status) {
			char* utf8 = win32_utf16_to_utf8(reinterpret_cast<wchar_t*>(result));
			if (utf8) {
				put_here = utf8;
				delete[] utf8;
			} else {
				status = false;
			}
		}
		delete[] result;
	}

	if (reg_key)
		RegCloseKey(reg_key);

	delete[] wkey;

	return status;
}




// Note that this sets the type as REG_SZ (string). The accepted utf-8
// value is converted to utf-16 for storage.
inline bool win32_set_registry_value_string(HKEY base,
		const std::string& keydir, const std::string& key, const std::string& value)
{
	wchar_t* wkeydir = win32_utf8_to_utf16(keydir.c_str());
	if (!wkeydir)
		return false;

	HKEY reg_key = NULL;
	if (RegOpenKeyExW(base, wkeydir, 0, KEY_QUERY_VALUE, &reg_key) != ERROR_SUCCESS)
		return false;

	delete[] wkeydir;

	bool status = true;

	unsigned int value_size = 0;  // including terminating 00
	wchar_t* wvalue = win32_utf8_to_utf16(value.c_str(), &value_size);
	wchar_t* wkey = win32_utf8_to_utf16(key.c_str());
	if (!wvalue || !wkey) {
		status = false;
	}

	if (status) {
		status = (RegSetValueExW(reg_key, wkey, 0, REG_SZ,
				reinterpret_cast<BYTE*>(wvalue), value_size) == ERROR_SUCCESS);
	}

	delete[] wvalue;
	delete[] wkey;

	if (reg_key)
		RegCloseKey(reg_key);

	return status;
}



// Redirect stdout and stderr to console window (if open).
inline bool win32_redirect_stdio_to_console(bool create_if_none)
{
	bool console_created = false;
	return win32_redirect_stdio_to_console(create_if_none, console_created);
}



// Redirect stdout and stderr to console window (if open).
inline bool win32_redirect_stdio_to_console(bool create_if_none, bool& console_created)
{
	console_created = false;

	// AttachConsole is since winxp (note that mingw and MS headers say win2k,
	// which is incorrect; msdn is correct).
#if defined WINVER && WINVER >= 0x0501
	if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
		if (create_if_none) {
			// Even though msdn says that stdio is redirected to the new console,
			// it isn't so and we must do it manually.
			if (!AllocConsole()) {
				return false;
			} else {
				console_created = true;
			}
		} else {
			return false;
		}
	}

	// redirect unbuffered STDOUT to the console
	HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
	std::FILE* fp_out = _fdopen(_open_osfhandle((intptr_t)(h_out), _O_TEXT), "w");
	*stdout = *fp_out;
	std::setvbuf(stdout, 0, _IONBF, 0);

	// redirect unbuffered STDERR to the console
	HANDLE h_err = GetStdHandle(STD_ERROR_HANDLE);
	std::FILE* fp_err = _fdopen(_open_osfhandle((intptr_t)(h_err), _O_TEXT), "w");
	*stderr = *fp_err;
	std::setvbuf(stderr, 0, _IONBF, 0);

	std::ios::sync_with_stdio();

	std::fprintf(stderr, "\n");  // so that we start with an empty line

	return true;

#else  // unsupported
	return false;
#endif
}




namespace internal {


	template<typename Dummy>
	struct Win32RedirectStaticHolder {
		static const wchar_t* stdout_file;
		static const wchar_t* stderr_file;
	};

	// definitions
	template<typename Dummy> const wchar_t* Win32RedirectStaticHolder<Dummy>::stdout_file = 0;
	template<typename Dummy> const wchar_t* Win32RedirectStaticHolder<Dummy>::stderr_file = 0;

	typedef Win32RedirectStaticHolder<void> Win32RedirectHolder;  // one (and only) specialization.




	/// Returns full path (in utf-16) to the output filename (in utf-8)
	inline wchar_t* win32_get_std_output_file(const char* base)
	{
		if (!base)
			return 0;

		// GetModuleFileName() is since win2k
	#if defined WINVER && WINVER >= 0x0500
		wchar_t name[MAX_PATH] = {0};

		// This function doesn't write terminating 0 if the buffer is insufficient,
		// so we reserve some space for it.
		if (GetModuleFileNameW(0, name, MAX_PATH - 1)) {
			std::string name_str = win32_utf16_to_utf8_string(name);
			if (!name_str.empty()) {
				std::string::size_type pos = name_str.find_last_of(".");
				name_str = name_str.substr(0, pos);  // full path, including the executable without extension.
				if (!name_str.empty()) {
					return win32_utf8_to_utf16((name_str + "-" + base).c_str());
				}
			}
			// std::string::size_type pos = name_str.find_last_of("\\/");
			// if (pos != std::string::npos && name_str.size() > pos + 1) {
			// 	name_str.substr(pos + 1);
			// }
		}
	#endif

		return win32_utf8_to_utf16(base);
	}



	/// Remove the output files if there was no output written.
	/// This is an atexit() callback.
	extern "C" inline void win32_redirect_stdio_to_files_cleanup()
	{
		// Flush the output in case anything is queued
		std::fclose(stdout);
		std::fclose(stderr);

		// See if the files have any output in them
		if (Win32RedirectHolder::stdout_file && Win32RedirectHolder::stdout_file[0] != 0) {
			std::FILE* file = 0;
		#if defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
			errno = _wfopen_s(&file, Win32RedirectHolder::stdout_file, L"rb");
		#else
			file = _wfopen(Win32RedirectHolder::stdout_file, L"rb");
		#endif
			if (file) {
				bool empty = (std::fgetc(file) == EOF);
				std::fclose(file);
				if (empty) {
					_wremove(Win32RedirectHolder::stdout_file);
				}
			}
		}
		delete[] Win32RedirectHolder::stdout_file;
		Win32RedirectHolder::stdout_file = 0;

		if (Win32RedirectHolder::stdout_file && Win32RedirectHolder::stderr_file[0] != 0) {
			std::FILE* file = 0;
		#if defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
			errno = _wfopen_s(&file, Win32RedirectHolder::stderr_file, L"rb");
		#else
			file = _wfopen(Win32RedirectHolder::stderr_file, L"rb");
		#endif
			if (file) {
				bool empty = (std::fgetc(file) == EOF);
				std::fclose(file);
				if (empty) {
					_wremove(Win32RedirectHolder::stderr_file);
				}
			}
		}
		delete[] Win32RedirectHolder::stderr_file;
		Win32RedirectHolder::stderr_file = 0;
	}


}  // ns




// Redirect stdout to stdout.txt and stderr to stderr.txt.
// Call once.
inline bool win32_redirect_stdio_to_files(std::string stdout_file, std::string stderr_file)
{
	wchar_t* wstdout_file = 0;  // freed on cleanup
	if (stdout_file.empty()) {
		wstdout_file = internal::win32_get_std_output_file("stdout.txt");
	} else {
		wstdout_file = win32_utf8_to_utf16(stdout_file.c_str());
	}
	std::FILE* fp_out = 0;
	if (wstdout_file) {
#if defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
		errno = _wfopen_s(&fp_out, wstdout_file, L"wb");
#else
		fp_out = _wfopen(wstdout_file, L"wb");
#endif
	}
	if (wstdout_file && fp_out) {
		*stdout = *fp_out;
		std::setvbuf(stdout, 0, _IOLBF, BUFSIZ);  // Line buffered
	} else {
		delete[] wstdout_file;
	}
	internal::Win32RedirectHolder::stdout_file = wstdout_file;  // so that it's freed properly

	wchar_t* wstderr_file = 0;  // freed on cleanup
	if (stderr_file.empty()) {
		wstderr_file = internal::win32_get_std_output_file("stderr.txt");
	} else {
		wstderr_file = win32_utf8_to_utf16(stderr_file.c_str());
	}
	std::FILE* fp_err = 0;
	if (wstderr_file) {
#if defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
		errno = _wfopen_s(&fp_err, wstderr_file, L"wb");
#else
		fp_err = _wfopen(wstderr_file, L"wb");
#endif
	}
	if (wstderr_file && fp_out) {
		*stderr = *fp_err;
		std::setvbuf(stderr, 0, _IONBF, BUFSIZ);  // No buffering
	} else {
		delete[] wstderr_file;
	}
	internal::Win32RedirectHolder::stderr_file = wstderr_file;  // so that it's freed properly

	if (fp_out || fp_err) {
		// make sure cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
		// write there as well.
		std::ios::sync_with_stdio();

		std::atexit(internal::win32_redirect_stdio_to_files_cleanup);

		return true;
	}

	return false;
}





// Despite what MSDN says, these MultiByteToWideChar and
// WideCharToMultiByte are even on win95.


// Convert a string in user-specified encoding to utf-16 string (00-terminated array of wchar_t).
// wchar_t is 2 bytes on windows, thus holding utf-16 code unit.
// Caller must delete[] the returned value.
// returned_buf_size (if not 0) will be set to the size of returned buffer
// (in wchar_t; including terminating 00). The byte size will be (returned_buf_size*2).
inline wchar_t* win32_user_to_utf16(UINT from_cp, const char* from_str, unsigned int* returned_buf_size)
{
	if (!from_str)
		return 0;

	int wide_bufsize = MultiByteToWideChar(from_cp, 0, from_str, -1, 0, 0);  // including terminating 0
	if (wide_bufsize == ERROR_NO_UNICODE_TRANSLATION) {
		return 0;  // invalid sequence
	}
	if (wide_bufsize == 0) {
		return 0;  // error in conversion
	}

	wchar_t* res = new wchar_t[wide_bufsize];
	int conv_size = MultiByteToWideChar(from_cp, 0, from_str, -1, res, wide_bufsize);

	if (conv_size != wide_bufsize) {
		delete[] res;
		return 0;  // ?
	}
	if (returned_buf_size) {
		*returned_buf_size = wide_bufsize;
	}
	return res;
}



// Convert utf-16 string (represented as 0-terminated array of wchar_t, where
// wchar_t is 2 bytes (on windows)) to a string in user-specified encoding.
// Caller must delete[] the returned value.
// The size of the returned buffer is std::strlen(buf) + 1.
// returned_buf_size (if not 0) will be set to the size of returned buffer
// (in char; including terminating 0).
inline char* win32_utf16_to_user(UINT to_cp, const wchar_t* utf16_str, unsigned int* returned_buf_size)
{
	if (!utf16_str)
		return 0;

	int buf_size = WideCharToMultiByte(to_cp, 0, utf16_str, -1, 0, 0, 0, 0);
	if (buf_size == 0) {
		return 0;
	}

	char* res = new char[buf_size];
	int conv_size = WideCharToMultiByte(to_cp, 0, utf16_str, -1, res, buf_size, 0, 0);

	if (conv_size != buf_size) {
		delete[] res;
		return 0;  // ?
	}
	if (returned_buf_size) {
		*returned_buf_size = buf_size;
	}
	return res;
}



// Convert utf-8 string to utf-16 string. See win32_user_to_utf16() for details.
inline wchar_t* win32_utf8_to_utf16(const char* utf8_str, unsigned int* returned_buf_size)
{
	return win32_user_to_utf16(CP_UTF8, utf8_str, returned_buf_size);
}


// Convert utf-16 string to utf-8 string. See win32_utf16_to_user() for details.
inline char* win32_utf16_to_utf8(const wchar_t* utf16_str, unsigned int* returned_buf_size)
{
	return win32_utf16_to_user(CP_UTF8, utf16_str, returned_buf_size);
}



// Same as win32_utf16_to_utf8(), but safer since it never returns 0 (it returns "" instead).
// Note that we don't provide non-utf8 versions of this function, because the other charsets
// may actually be multi-byte (std::string can't hold their terminating 0 properly).
// Also, we don't provide std::wstring versions for utf16, because they are not always
// supported by non-MS compilers (mingw, for example).
inline std::string win32_utf16_to_utf8_string(const wchar_t* utf16_str)
{
	char* buf = win32_utf16_to_utf8(utf16_str);
	std::string ret(buf ? buf : "");
	delete[] buf;
	return ret;
}



// Convert current locale-encoded string to utf-16 string. See win32_user_to_utf16() for details.
// Use CP_ACP is system default windows ANSI codepage.
// Use CP_THREAD_ACP is for current thread. Think before you choose. :)
inline wchar_t* win32_locale_to_utf16(const char* loc_str, unsigned int* returned_buf_size,
		bool use_thread_locale)
{
	return win32_user_to_utf16(use_thread_locale ? CP_THREAD_ACP : CP_ACP, loc_str, returned_buf_size);
}


// Convert utf-16 string to locale-encoded string. See win32_utf16_to_user() for details.
inline char* win32_utf16_to_locale(const wchar_t* utf16_str, unsigned int* returned_buf_size,
		bool use_thread_locale)
{
	return win32_utf16_to_user(use_thread_locale ? CP_THREAD_ACP : CP_ACP, utf16_str, returned_buf_size);
}



// Convert current locale-encoded string to utf-8 string. The returned value must be freed by caller.
inline char* win32_locale_to_utf8(const char* loc_str, unsigned int* returned_buf_size,
		bool use_thread_locale)
{
	wchar_t* utf16_str = win32_locale_to_utf16(loc_str, 0, use_thread_locale);
	char* utf8_str = win32_utf16_to_utf8(utf16_str, returned_buf_size);
	delete[] utf16_str;
	return utf8_str;
}


// Convert utf-8 string to locale-encoded string. See win32_utf8_to_user() for details.
inline char* win32_utf8_to_locale(const char* utf8_str, unsigned int* returned_buf_size,
		bool use_thread_locale)
{
	wchar_t* utf16_str = win32_utf8_to_utf16(utf8_str);
	char* loc_str = win32_utf16_to_locale(utf16_str, returned_buf_size, use_thread_locale);
	delete[] utf16_str;
	return loc_str;
}










}  // ns



#endif  // win32

#endif  // hg

/// @}
