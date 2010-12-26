/**************************************************************************
 Copyright:
      (C) 2003 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_WIN32_TOOLS_H
#define HZ_WIN32_TOOLS_H

#include "hz_config.h"  // feature macros

#ifdef _WIN32  // Guards this header completely

#include <windows.h>
#include <string>
#include <cctype>  // toupper



namespace hz {



template<class Container> inline
bool win32_get_drive_list(Container& put_here)
{
	DWORD buf_size = GetLogicalDriveStrings(0, 0);
	if (!buf_size)  // error
		return false;

	char* buf = new char[buf_size];

	if (!GetLogicalDriveStrings(sizeof(buf), buf)) {
		delete[] buf;
		return false;
	}

	char* text_ptr = buf;

	// add the drives to list
	while (*text_ptr != '\0') {
		std::string str;
		str += static_cast<char>(std::toupper(text_ptr[0]));
		str += ":\\";
		put_here.push_back(str);

		while (*text_ptr != '\0')
			++text_ptr;
		++text_ptr;  // set to position after 0
	}

	delete[] buf;

	return true;
}




// base may be e.g. HKEY_CURRENT_USER
inline bool win32_get_registry_value_string(std::string& put_here, HKEY base,
		const std::string& keydir, const std::string& key)
{
	HKEY reg_key = NULL;
	if (RegOpenKeyEx(base, keydir.c_str(), 0, KEY_QUERY_VALUE, &reg_key) != ERROR_SUCCESS)
		return false;

	DWORD type = 0;
	DWORD nbytes = 0;
	bool status = (RegQueryValueEx(reg_key, key.c_str(), 0, &type, NULL, &nbytes) == ERROR_SUCCESS);

	if (status) {
		char* result = new char[nbytes + 1];
		status = (RegQueryValueEx(reg_key, key.c_str(), 0, &type, (BYTE*)result, &nbytes) == ERROR_SUCCESS);
		result[nbytes] = '\0';
		put_here = result;
		delete[] result;
	}

	if (reg_key)
		RegCloseKey(reg_key);

	return status;
}




inline bool win32_set_registry_value_string(HKEY base,
		const std::string& keydir, const std::string& key, const std::string& value)
{
	HKEY reg_key = NULL;
	DWORD nbytes = value.length() + 1;

	if (RegOpenKeyEx(base, keydir.c_str(), 0, KEY_QUERY_VALUE, &reg_key) != ERROR_SUCCESS)
		return false;

	bool status = (RegSetValueEx(reg_key, key.c_str(), 0, REG_SZ,
			(const BYTE*)(value.c_str()), nbytes) == ERROR_SUCCESS);

	if (reg_key)
		RegCloseKey (reg_key);

	return status;
}




}  // ns



#endif  // win32

#endif  // hg
