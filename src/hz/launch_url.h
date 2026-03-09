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

#ifndef HZ_LAUNCH_URL_H
#define HZ_LAUNCH_URL_H

#include <string>
#include <gtk/gtk.h>

#ifdef _WIN32
	#include <windows.h>  // seems to be needed by shellapi.h
	#include <shellapi.h>  // ShellExecuteW()
	#include "win32_tools.h"  // hz::win32_utf8_to_utf16
#else
	#include <memory>
	#include <unistd.h>  // geteuid, fork, execvp, setuid, setgid
	#include <sys/types.h>  // uid_t, gid_t
	#include <sys/wait.h>  // waitpid
	#include <pwd.h>  // getpwuid
	#include "env_tools.h"  // hz::env_get_value
#endif




namespace hz {


#ifndef _WIN32

/// Launch URL as the original user when running as root.
/// This is needed because gtk_show_uri_on_window() doesn't work when running as root
/// (D-Bus session is not accessible).
/// \return error message on error, empty string on success.
inline std::string launch_url_as_original_user(const std::string& link)
{
	// Get the original user's UID from environment variables
	// SUDO_UID is set by sudo, PKEXEC_UID is set by pkexec
	std::string uid_str;
	uid_t original_uid = 0;
	gid_t original_gid = 0;

	if (hz::env_get_value("SUDO_UID", uid_str) || hz::env_get_value("PKEXEC_UID", uid_str)) {
		try {
			original_uid = static_cast<uid_t>(std::stoul(uid_str));
		} catch (...) {
			return "Cannot parse original user UID";
		}

		// Get the original user's GID
		struct passwd* pw = getpwuid(original_uid);
		if (pw) {
			original_gid = pw->pw_gid;
		} else {
			return "Cannot get original user information";
		}
	} else {
		return "Cannot determine original user UID";
	}

	// Fork and execute xdg-open as the original user
	pid_t pid = fork();
	if (pid < 0) {
		return "Cannot fork process";
	}

	if (pid == 0) {
		// Child process

		// Restore HOME environment variable if available
		// This helps xdg-open find the correct configuration
		std::string sudo_user;
		if (hz::env_get_value("SUDO_USER", sudo_user)) {
			struct passwd* pw = getpwnam(sudo_user.c_str());
			if (pw && pw->pw_dir) {
				setenv("HOME", pw->pw_dir, 1);
			}
		}

		// Drop privileges to original user
		// Set GID first, then UID (order matters for security)
		if (setgid(original_gid) != 0) {
			_exit(1);
		}
		if (setuid(original_uid) != 0) {
			_exit(1);
		}

		// Execute xdg-open with the URL
		const char* argv[] = {"xdg-open", link.c_str(), nullptr};
		execvp("xdg-open", const_cast<char* const*>(argv));

		// If execvp returns, it failed
		_exit(1);
	}

	// Parent process - wait for child
	int status = 0;
	if (waitpid(pid, &status, 0) == -1) {
		return "Cannot wait for child process";
	}

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		return {};  // Success
	}

	return "xdg-open failed to launch URL";
}

#endif  // _WIN32


/// Open URL in browser or mailto: link in mail client.
/// Return error message on error, empty string otherwise.
/// The link is in utf-8 in windows.
inline std::string launch_url([[maybe_unused]] GtkWindow* window, const std::string& link)
{
	// For some reason, gtk_show_uri() crashes on windows, so use our manual implementation.
#ifdef _WIN32
	std::wstring wlink = hz::win32_utf8_to_utf16(link);
	if (wlink.empty())
		return "Error while executing a command: The specified URI contains non-UTF-8 characters.";

	HINSTANCE inst = ShellExecuteW(nullptr, L"open", wlink.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	if (inst > reinterpret_cast<HINSTANCE>(32)) {
		return std::string();
	}
	return "Error while executing a command: Internal error.";

#else

	GError* error = nullptr;
	bool status = false;

	// Check if running as root
	bool is_root = (geteuid() == 0);

	// If running as root, try to launch as the original user first
	if (is_root) {
		std::string result = launch_url_as_original_user(link);
		if (result.empty()) {
			return {};  // Success
		}
		// If launching as original user failed, fall through to try GTK method
	}

	// Try the standard GTK method
#if GTK_CHECK_VERSION(3, 22, 0)
	status = static_cast<bool>(gtk_show_uri_on_window(window, link.c_str(), GDK_CURRENT_TIME, &error));
#else
	GdkScreen* screen = (window ? gtk_window_get_screen(window) : nullptr);
	status = static_cast<bool>(gtk_show_uri(screen, link.c_str(), GDK_CURRENT_TIME, &error));
#endif
	std::unique_ptr<GError, decltype(&g_error_free)> uerror(error, &g_error_free);

	if (!status) {
		// GTK method failed. If running as root, we already tried the fallback.
		// Otherwise, try the fallback now.
		if (!is_root) {
			std::string result = launch_url_as_original_user(link);
			if (result.empty()) {
				return {};  // Success
			}
		}

		// Both methods failed, return error
		return std::string("Cannot open URL")
				+ ((error && error->message) ? (std::string(": ") + error->message) : ".");
	}
	return {};
#endif
}




}  // ns





#endif

/// @}
