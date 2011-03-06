/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include "storage_detector_win32.h"

#if defined CONFIG_KERNEL_FAMILY_WINDOWS

#include <windows.h>  // CreateFileA(), CloseHandle(), etc...

#include "hz/string_sprintf.h"



// smartctl accepts various variants, the most straight being pdN,
// (or /dev/pdN, /dev/ being optional) where N comes from
// "\\.\PhysicalDriveN" (winnt only).
// http://msdn.microsoft.com/en-us/library/aa365247(VS.85).aspx

std::string detect_drives_win32(std::vector<std::string>& devices)
{
	for (int drive_num = 0; ; ++drive_num) {
		std::string name = hz::string_sprintf("\\\\.\\PhysicalDrive%d", drive_num);

		// If the drive is openable, then it's there.
		// NOTE: Administrative privileges are required to open it.
		// Yes, CreateFile() is open, not create. Yes, it's silly (ah, win32...).
		// We don't use any long/unopenable files here, so use the ANSI version.
		HANDLE h = CreateFileA(name.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, OPEN_EXISTING, 0, NULL);

		// The numbers seem to be consecutive, so break on first invalid.
		if (h == INVALID_HANDLE_VALUE)
			break;

		CloseHandle(h);

		devices.push_back(hz::string_sprintf("pd%d", drive_num));
	}

	return std::string();
}




#endif  // CONFIG_KERNEL_FAMILY_WINDOWS
