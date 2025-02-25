/******************************************************************************
License: Zlib
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_WIN32_TOOLS_H
#define HZ_WIN32_TOOLS_H

#ifdef _WIN32  // Guards this header completely

#include <windows.h>  // lots of stuff
#include <shlobj.h>  // SH*
#include <io.h>  // _open_osfhandle, _waccess
#include <fcntl.h>  // _O_TEXT

#include <string>
#include <vector>
#include <array>
#include <cctype>  // for ctype.h
#include <ctype.h>  // towupper
#include <cstdio>  // for stdio.h, std::f*, std::setvbuf, std::fprintf, std::FILE
#include <stdio.h>  // _fdopen, _wfopen, _wremove
#include <cstdlib>  // std::atexit
#include <cstring>  // std::strlen
#include <ios>  // std::ios::sync_with_stdio
#include <filesystem>

#if defined __MINGW32__
	#include <_mingw.h>  // MINGW_HAS_SECURE_API
#endif



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
inline bool win32_redirect_stdio_to_files(const std::string& stdout_file = "", const std::string& stderr_file = "");




/// Convert a string in user-specified encoding to utf-16 string (00-terminated array of wchar_t).
/// wchar_t is 2 bytes on windows, thus holding utf-16 code unit.
inline std::wstring win32_user_to_utf16(UINT from_cp, std::string_view from_str, bool* ok = nullptr);


/// Convert utf-16 string (represented as 0-terminated array of wchar_t, where
/// wchar_t is 2 bytes (on windows)) to a string in user-specified encoding.
inline std::string win32_utf16_to_user(UINT to_cp, std::wstring_view utf16_str, bool* ok = nullptr);


/// Convert utf-8 string to utf-16 string. See win32_user_to_utf16() for details.
inline std::wstring win32_utf8_to_utf16(std::string_view utf8_str, bool* ok = nullptr);



/// Convert utf-16 string to utf-8 string. See win32_utf16_to_user() for details.
inline std::string win32_utf16_to_utf8(std::wstring_view utf16_str, bool* ok = nullptr);

/// Convert utf-16 string to utf-8 string. See win32_utf16_to_user() for details.
template<std::size_t ArraySize>
std::string win32_utf16_to_utf8(const std::array<wchar_t, ArraySize>& utf16_str, bool* ok = nullptr);



/// Convert current locale-encoded string to utf-16 string. See win32_user_to_utf16() for details.
/// Use CP_ACP is system default windows ANSI codepage.
/// Use CP_THREAD_ACP is for current thread. Think before you choose. :)
inline std::wstring win32_locale_to_utf16(std::string_view loc_str, bool use_thread_locale = false, bool* ok = nullptr);


/// Convert utf-16 string to locale-encoded string. See win32_utf16_to_user() for details.
inline std::string win32_utf16_to_locale(std::wstring_view utf16_str, bool use_thread_locale = false, bool* ok = nullptr);


/// Convert current locale-encoded string to utf-8 string.
inline std::string win32_locale_to_utf8(std::string_view loc_str, bool use_thread_locale = false, bool* ok = nullptr);


/// Convert utf-8 string to locale-encoded string. See win32_utf8_to_user() for details.
inline std::string win32_utf8_to_locale(std::string_view utf8_str, bool use_thread_locale = false, bool* ok = nullptr);






// ------------------------------------------- Implementation




// Get a list of drives available for the user. The drives are in
// "C:\" format (uppercase, utf-8).
template<class Container> inline
bool win32_get_drive_list(Container& put_here)
{
	DWORD buf_size = GetLogicalDriveStringsW(0, nullptr);  // find out the required buffer size
	if (buf_size == 0)  // error
		return false;

	std::vector<wchar_t> buf(buf_size);

	// Returns a consecutive array of 00-terminated strings (each 00-terminated).
	if (!GetLogicalDriveStringsW(buf_size, buf.data())) {
		return false;
	}

	wchar_t* text_ptr = buf.data();

	// add the drives to list
	while (*text_ptr != 0) {
		text_ptr[0] = static_cast<wchar_t>(towupper(text_ptr[0]));
		std::string drive = win32_utf16_to_utf8(text_ptr);
		if (!drive.empty()) {
			put_here.push_back(drive + ":\\");
		}
		while (*text_ptr != 0)
			++text_ptr;
		++text_ptr;  // set to position after 0
	}

	return true;
}



// Get windows "special" folder by CSIDL (CSIDL_APPDATA, for example).
// See http://msdn.microsoft.com/en-us/library/bb762494(VS.85).aspx
// for details.
inline std::string win32_get_special_folder(int csidl, bool auto_create)
{
	std::array<wchar_t, MAX_PATH> buf{};

	// SHGetSpecialFolderPath() is since ie4
#if (defined _WIN32_IE) && (_WIN32_IE >= 0x0400)
	if (SHGetSpecialFolderPathW(nullptr, buf.data(), csidl, static_cast<BOOL>(auto_create)) == TRUE) {
		return win32_utf16_to_utf8(buf);
	}

#else
	LPITEMIDLIST pidl = nullptr;

	if (SHGetSpecialFolderLocation(nullptr, csidl, &pidl) == S_OK) {
		if (SHGetPathFromIDListW(pidl, buf.data())== TRUE) {
			errno = 0;
			// 00 is F_OK (for msvc). ENOENT is "no such file or directory".
			// We can't really do anything if errors happen here, so ignore them.
			if (auto_create && _waccess(buf.data(), 00) == -1 && errno == ENOENT) {
				_wmkdir(buf.data());
			}
			return win32_utf16_to_utf8(buf);
		}

		// We must free pidl. This can be done in two ways:

		// CoTaskMemFree() requires linking to ole32.
		// CoTaskMemFree(pidl);

		// SHGetMalloc() which is deprecated and may be removed in the future.
		IMalloc* allocator = nullptr;
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
	std::array<wchar_t, MAX_PATH> buf{};
	UINT status = GetWindowsDirectoryW(buf.data(), MAX_PATH);
	if (status != 0 && buf[0] != 0) {
		win32_utf16_to_utf8(buf);
	}
	return "C:\\";
}



// base may be e.g. HKEY_CURRENT_USER.
// Note that this works only with REG_SZ (string) types,
// returning their value as utf-8. False is returned for all other types.
inline bool win32_get_registry_value_string(HKEY base,
		const std::string& keydir, const std::string& key, std::string& put_here)
{
	std::wstring wkeydir = win32_utf8_to_utf16(keydir);
	if (wkeydir.empty())
		return false;

	HKEY reg_key = nullptr;
	bool open_status = (RegOpenKeyExW(base, wkeydir.c_str(), 0, KEY_QUERY_VALUE, &reg_key) == ERROR_SUCCESS);

	if (!open_status)
		return false;

	bool ok = false;
	std::wstring wkey = win32_utf8_to_utf16(key, &ok);
	if (!ok) {  // conversion error. Note that an empty string is not an error.
		if (reg_key)
			RegCloseKey(reg_key);
		return false;
	}

	DWORD type = 0;
	DWORD nbytes = 0;
	// This returns the size, including the 0 character only if the data was stored with it.
	bool status = (RegQueryValueExW(reg_key, wkey.c_str(), nullptr, &type, nullptr, &nbytes) == ERROR_SUCCESS);

	if (status) {
		DWORD buf_len = ((nbytes % 2) == 1 ? (nbytes+1) : nbytes);  // 00-terminate on 2-byte boundary, in case the value isn't.
		std::vector<BYTE> result(buf_len);

		status = (RegQueryValueExW(reg_key, wkey.c_str(), nullptr, &type, result.data(), &nbytes) == ERROR_SUCCESS);

		if (status) {
			std::string utf8 = win32_utf16_to_utf8(reinterpret_cast<wchar_t*>(result.data()), &ok);
			if (ok) {
				put_here = utf8;
			} else {
				status = false;
			}
		}
	}

	if (reg_key)
		RegCloseKey(reg_key);

	return status;
}




// Note that this sets the type as REG_SZ (string). The accepted utf-8
// value is converted to utf-16 for storage.
inline bool win32_set_registry_value_string(HKEY base,
		const std::string& keydir, const std::string& key, const std::string& value)
{
	std::wstring wkeydir = win32_utf8_to_utf16(keydir);
	if (wkeydir.empty())
		return false;

	HKEY reg_key = nullptr;
	if (RegOpenKeyExW(base, wkeydir.c_str(), 0, KEY_QUERY_VALUE, &reg_key) != ERROR_SUCCESS)
		return false;

	bool status = true;

	bool ok = false;
	std::wstring wvalue = win32_utf8_to_utf16(value, &ok);
	std::wstring wkey;
	if (ok) {
		wkey = win32_utf8_to_utf16(key, &ok);
	}
	if (!ok) {
		status = false;
	}

	if (status) {
		status = (RegSetValueExW(reg_key, wkey.c_str(), 0, REG_SZ,
					reinterpret_cast<const BYTE*>(wvalue.data()),
					DWORD(wvalue.size() + 1*sizeof(wchar_t))
				) == ERROR_SUCCESS);
	}

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
	if (AttachConsole(ATTACH_PARENT_PROCESS) == FALSE) {
		if (create_if_none) {
			// Even though msdn says that stdio is redirected to the new console,
			// it isn't so and we must do it manually.
			if (AllocConsole() == FALSE) {
				return false;
			}
			console_created = true;
		} else {
			return false;
		}
	}

	// redirect unbuffered STDOUT to the console
	HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
	std::FILE* fp_out = _fdopen(_open_osfhandle((intptr_t)(h_out), _O_TEXT), "w");
	*stdout = *fp_out;
	std::setvbuf(stdout, nullptr, _IONBF, 0);

	// redirect unbuffered STDERR to the console
	HANDLE h_err = GetStdHandle(STD_ERROR_HANDLE);
	std::FILE* fp_err = _fdopen(_open_osfhandle((intptr_t)(h_err), _O_TEXT), "w");
	*stderr = *fp_err;
	std::setvbuf(stderr, nullptr, _IONBF, 0);

	std::ios::sync_with_stdio();

	std::fprintf(stderr, "\n");  // so that we start with an empty line

	return true;

#else  // unsupported
	return false;
#endif
}




namespace internal {


	struct Win32RedirectHolder {
		static inline std::wstring stdout_file;
		static inline std::wstring stderr_file;
	};



	/// Returns full path (in utf-16) to the output filename (in utf-8)
	inline std::wstring win32_get_std_output_file(std::wstring_view base)
	{
		std::array<wchar_t, MAX_PATH> name{};

		// This function doesn't write terminating 0 if the buffer is insufficient,
		// so we reserve some space for it.
		if (GetModuleFileNameW(nullptr, name.data(), MAX_PATH - 1) == TRUE) {
			std::wstring wname(name.data());
			std::wstring::size_type pos = wname.find_last_of(L'.');
			wname = wname.substr(0, pos);  // full path, including the executable without extension.
			if (!wname.empty()) {
				return wname + L"-" + std::wstring(base);
			}
		}

		return std::wstring(base);
	}



	/// Remove the output files if there was no output written.
	/// This is an atexit() callback.
	extern "C" inline void win32_redirect_stdio_to_files_cleanup()
	{
		// Flush the output in case anything is queued
		std::fclose(stdout);
		std::fclose(stderr);

		// See if the files have any output in them. If not, remove the empty files.
		if (!Win32RedirectHolder::stdout_file.empty()) {
			std::filesystem::path path(Win32RedirectHolder::stdout_file);
			std::error_code ignored_ec;
			if (std::filesystem::exists(path, ignored_ec) && std::filesystem::is_empty(path, ignored_ec)) {
				std::filesystem::remove(path, ignored_ec);
			}
			Win32RedirectHolder::stdout_file.clear();
		}
		if (!Win32RedirectHolder::stderr_file.empty()) {
			std::filesystem::path path(Win32RedirectHolder::stderr_file);
			std::error_code ignored_ec;
			if (std::filesystem::exists(path, ignored_ec) && std::filesystem::is_empty(path, ignored_ec)) {
				std::filesystem::remove(path, ignored_ec);
			}
			Win32RedirectHolder::stderr_file.clear();
		}
	}


}  // ns




// Redirect stdout to stdout.txt and stderr to stderr.txt.
// Call once.
inline bool win32_redirect_stdio_to_files(const std::string& stdout_file, const std::string& stderr_file)
{
	std::wstring wstdout_file;
	if (stdout_file.empty()) {
		wstdout_file = internal::win32_get_std_output_file(L"stdout.txt");
	} else {
		wstdout_file = win32_utf8_to_utf16(stdout_file);
	}
	std::FILE* fp_out = nullptr;
	if (!wstdout_file.empty()) {
#if defined MINGW_HAS_SECURE_API || defined _MSC_VER
		errno = _wfopen_s(&fp_out, wstdout_file.c_str(), L"wb");
#else
		fp_out = _wfopen(wstdout_file.c_str(), L"wb");
#endif
	}
	if (!wstdout_file.empty() && fp_out) {
		*stdout = *fp_out;
		std::setvbuf(stdout, nullptr, _IOLBF, BUFSIZ);  // Line buffered
	} else {
		wstdout_file.clear();
	}
	internal::Win32RedirectHolder::stdout_file = wstdout_file;

	std::wstring wstderr_file;  // freed on cleanup
	if (stderr_file.empty()) {
		wstderr_file = internal::win32_get_std_output_file(L"stderr.txt");
	} else {
		wstderr_file = win32_utf8_to_utf16(stderr_file);
	}
	std::FILE* fp_err = nullptr;
	if (!wstderr_file.empty()) {
#if defined MINGW_HAS_SECURE_API || defined _MSC_VER
		errno = _wfopen_s(&fp_err, wstderr_file.c_str(), L"wb");
#else
		fp_err = _wfopen(wstderr_file.c_str(), L"wb");
#endif
	}
	if (!wstderr_file.empty() && fp_out) {
		*stderr = *fp_err;
		std::setvbuf(stderr, nullptr, _IONBF, BUFSIZ);  // No buffering
	} else {
		wstderr_file.clear();
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






inline std::wstring win32_user_to_utf16(UINT from_cp, std::string_view from_str, bool* ok)
{
	if (from_str.empty()) {
		if (ok) {
			*ok = true;
		}
		return std::wstring();
	}

	// excluding terminating 0
	int wide_bufsize = MultiByteToWideChar(from_cp, 0, from_str.data(), int(from_str.size()), nullptr, 0);
	if (wide_bufsize == ERROR_NO_UNICODE_TRANSLATION) {
		if (ok) {
			*ok = false;
		}
		return std::wstring();  // invalid sequence
	}
	if (wide_bufsize == 0) {
		if (ok) {
			*ok = false;
		}
		return std::wstring();  // error in conversion
	}

	std::vector<wchar_t> res(std::size_t(wide_bufsize + 1));
	MultiByteToWideChar(from_cp, 0, from_str.data(), int(from_str.size()), res.data(), wide_bufsize + 1);
	res.back() = L'\0';  // just in case

	if (ok) {
		*ok = true;
	}
	return std::wstring(res.data());
}



// Convert utf-16 string (represented as 0-terminated array of wchar_t, where
// wchar_t is 2 bytes (on windows)) to a string in user-specified encoding.
// Caller must delete[] the returned value.
// The size of the returned buffer is std::strlen(buf) + 1.
// returned_buf_size (if not 0) will be set to the size of returned buffer
// (in char; including terminating 0).
inline std::string win32_utf16_to_user(UINT to_cp, std::wstring_view utf16_str, bool* ok)
{
	if (utf16_str.empty()) {
		if (ok) {
			*ok = true;
		}
		return std::string();
	}

	int buf_size = WideCharToMultiByte(to_cp, 0, utf16_str.data(), int(utf16_str.size()), nullptr, 0, nullptr, nullptr);
	if (buf_size == 0) {
		if (ok) {
			*ok = false;
		}
		return std::string();
	}

	std::vector<char> res(std::size_t(buf_size + 1));
	WideCharToMultiByte(to_cp, 0,
			utf16_str.data(), int(utf16_str.size()),
			res.data(), int(res.size()),
			nullptr, nullptr);
	res.back() = '\0';  // just in case

	if (ok) {
		*ok = true;
	}
	return std::string(res.data());
}



// Convert utf-8 string to utf-16 string. See win32_user_to_utf16() for details.
inline std::wstring win32_utf8_to_utf16(std::string_view utf8_str, bool* ok)
{
	return win32_user_to_utf16(CP_UTF8, utf8_str, ok);
}


// Convert utf-16 string to utf-8 string. See win32_utf16_to_user() for details.
inline std::string win32_utf16_to_utf8(std::wstring_view utf16_str, bool* ok)
{
	return win32_utf16_to_user(CP_UTF8, utf16_str, ok);
}



// Convert utf-16 string to utf-8 string. See win32_utf16_to_user() for details.
template<std::size_t ArraySize>
std::string win32_utf16_to_utf8(const std::array<wchar_t, ArraySize>& utf16_str, bool* ok)
{
	return win32_utf16_to_user(CP_UTF8, {utf16_str.data()}, ok);
}



// Convert current locale-encoded string to utf-16 string. See win32_user_to_utf16() for details.
// Use CP_ACP is system default windows ANSI codepage.
// Use CP_THREAD_ACP is for current thread. Think before you choose. :)
inline std::wstring win32_locale_to_utf16(std::string_view loc_str, bool use_thread_locale, bool* ok)
{
	return win32_user_to_utf16(use_thread_locale ? CP_THREAD_ACP : CP_ACP, loc_str, ok);
}


// Convert utf-16 string to locale-encoded string. See win32_utf16_to_user() for details.
inline std::string win32_utf16_to_locale(std::wstring_view utf16_str, bool use_thread_locale, bool* ok)
{
	return win32_utf16_to_user(use_thread_locale ? CP_THREAD_ACP : CP_ACP, utf16_str, ok);
}



// Convert current locale-encoded string to utf-8 string.
inline std::string win32_locale_to_utf8(std::string_view loc_str, bool use_thread_locale, bool* ok)
{
	std::wstring utf16_str = win32_locale_to_utf16(loc_str, use_thread_locale, ok);
	if (ok && !(*ok)) {
		return std::string();
	}
	return win32_utf16_to_utf8(utf16_str, ok);
}


// Convert utf-8 string to locale-encoded string. See win32_utf8_to_user() for details.
inline std::string win32_utf8_to_locale(std::string_view utf8_str, bool use_thread_locale, bool* ok)
{
	std::wstring utf16_str = win32_utf8_to_utf16(utf8_str, ok);
	if (ok && !(*ok)) {
		return std::string();
	}
	return win32_utf16_to_locale(utf16_str, use_thread_locale, ok);
}










}  // ns



#endif  // win32

#endif  // hg

/// @}
