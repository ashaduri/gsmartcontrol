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
	#include <optional>
	#include <vector>
	#include <glib.h>  // g_spawn_async
	#include <unistd.h>  // geteuid, setuid, setgid, setgroups (BSD)
	#include <grp.h>  // setgroups (Linux), getgrouplist
	#include <pwd.h>  // getpwuid
	#include <sys/stat.h>  // stat
	#include <sys/types.h>  // uid_t, gid_t
	#include "env_tools.h"  // env_get_value
	#include "string_num.h"  // string_is_numeric_nolocale
#endif




namespace hz {


#ifndef _WIN32

namespace launch_url_internal {


	/// Unprivileged user that started the program, when it's running as root
	/// (via sudo, pkexec, kdesu, etc.).
	struct OriginalUser {
		uid_t uid = 0;
		gid_t gid = 0;
		std::vector<gid_t> groups;  ///< Supplementary groups
		std::string home;  ///< Home directory
		std::vector<std::string> env;  ///< Environment for processes launched as this user
	};


	/// Find the UID of the user that started the program as root.
	inline std::optional<uid_t> find_original_uid()
	{
		// pkexec and sudo tell us directly
		for (const char* var : {"PKEXEC_UID", "SUDO_UID"}) {
			std::string uid_str;
			uid_t uid = 0;
			if (env_get_value(var, uid_str) && string_is_numeric_nolocale(uid_str, uid, true, 10) && uid != 0) {
				return uid;
			}
		}
		// Other tools (kdesu, gksu, su-to-root, ...) keep XAUTHORITY to allow access to the
		// user's display. The file is owned by the user.
		std::string xauthority;
		if (env_get_value("XAUTHORITY", xauthority) && !xauthority.empty()) {
			struct stat st = {};
			if (stat(xauthority.c_str(), &st) == 0 && st.st_uid != 0) {
				return st.st_uid;
			}
		}
		return std::nullopt;
	}


	/// Get information about the original user, if running as root on behalf of another user.
	inline std::optional<OriginalUser> get_original_user()
	{
		if (geteuid() != 0) {
			return std::nullopt;
		}
		const auto uid = find_original_uid();
		if (!uid.has_value()) {
			return std::nullopt;
		}
		const struct passwd* pw = getpwuid(uid.value());
		if (!pw || !pw->pw_name) {
			return std::nullopt;
		}

		OriginalUser user;
		user.uid = pw->pw_uid;
		user.gid = pw->pw_gid;
		user.home = (pw->pw_dir ? pw->pw_dir : "/");
		const std::string name = pw->pw_name;

		// Supplementary groups, so that root's groups are not inherited.
		int num_groups = 64;
		for (int attempt = 0; attempt < 2; ++attempt) {
			user.groups.resize(static_cast<std::size_t>(num_groups));
#ifdef __APPLE__
			const int status = getgrouplist(name.c_str(), static_cast<int>(user.gid),
					reinterpret_cast<int*>(user.groups.data()), &num_groups);
#else
			const int status = getgrouplist(name.c_str(), user.gid, user.groups.data(), &num_groups);
#endif
			if (status >= 0) {
				user.groups.resize(static_cast<std::size_t>(num_groups));
				break;
			}
			if (attempt == 1) {  // still doesn't fit; use the primary group only
				user.groups = {user.gid};
			}
		}

		// Environment for the user's desktop session. Don't pass root's environment as is.
		user.env.push_back("HOME=" + user.home);
		user.env.push_back("USER=" + name);
		user.env.push_back("LOGNAME=" + name);
		user.env.push_back("PATH=/usr/local/bin:/usr/bin:/bin");
		for (const char* var : {"DISPLAY", "XAUTHORITY", "WAYLAND_DISPLAY", "XDG_CURRENT_DESKTOP",
				"XDG_SESSION_TYPE", "DESKTOP_SESSION", "LANG", "LANGUAGE", "LC_ALL", "LC_MESSAGES"}) {
			std::string value;
			if (env_get_value(var, value)) {
				user.env.push_back(std::string(var) + "=" + value);
			}
		}
		// The user's runtime directory and session bus are needed by xdg-open / browsers (and Wayland).
		const std::string runtime_dir = "/run/user/" + std::to_string(user.uid);
		struct stat st = {};
		if (stat(runtime_dir.c_str(), &st) == 0 && S_ISDIR(st.st_mode) && st.st_uid == user.uid) {
			user.env.push_back("XDG_RUNTIME_DIR=" + runtime_dir);
			const std::string bus = runtime_dir + "/bus";
			if (stat(bus.c_str(), &st) == 0 && st.st_uid == user.uid) {
				user.env.push_back("DBUS_SESSION_BUS_ADDRESS=unix:path=" + bus);
			}
		}

		return user;
	}


	/// Called in the child process between fork() and exec(). Only async-signal-safe
	/// calls are allowed here; all the data is prepared in advance.
	/// If the privileges cannot be dropped, the child exits instead of running as root.
	inline void drop_privileges_child_setup(gpointer data)
	{
		const auto* user = static_cast<const OriginalUser*>(data);
#ifdef __linux__
		const auto num_groups = user->groups.size();
#else
		const auto num_groups = static_cast<int>(user->groups.size());
#endif
		if (setgroups(num_groups, user->groups.data()) != 0
				|| setgid(user->gid) != 0 || setuid(user->uid) != 0) {
			_exit(127);
		}
		if (setuid(0) == 0) {  // make sure root cannot be regained
			_exit(127);
		}
	}


	/// Launch the URL with xdg-open as the original user.
	/// \return error message on error, empty string on success.
	inline std::string launch_url_as_user(const OriginalUser& user, const std::string& link)
	{
		std::vector<std::string> args = {"xdg-open", link};
		std::vector<gchar*> argv;
		for (auto& arg : args) {
			argv.push_back(arg.data());
		}
		argv.push_back(nullptr);

		std::vector<std::string> env = user.env;
		std::vector<gchar*> envp;
		for (auto& var : env) {
			envp.push_back(var.data());
		}
		envp.push_back(nullptr);

		// The child is reaped automatically (no G_SPAWN_DO_NOT_REAP_CHILD), and we don't
		// wait for it, since xdg-open may not return until the browser is closed.
		GError* error = nullptr;
		const bool status = static_cast<bool>(g_spawn_async(user.home.c_str(), argv.data(), envp.data(),
				GSpawnFlags(G_SPAWN_SEARCH_PATH_FROM_ENVP | G_SPAWN_STDOUT_TO_DEV_NULL),
				&drop_privileges_child_setup, const_cast<OriginalUser*>(&user), nullptr, &error));
		std::unique_ptr<GError, decltype(&g_error_free)> uerror(error, &g_error_free);

		if (!status) {
			return std::string("Cannot open URL")
					+ ((error && error->message) ? (std::string(": ") + error->message) : ".");
		}
		return {};
	}


}  // ns launch_url_internal

#endif  // !_WIN32



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

	// When running as root on behalf of a user (e.g. via gsmartcontrol-root), the browser should
	// run as that user: browsers refuse to run as root in a user's session, and the user's
	// desktop session (D-Bus, etc.) is not accessible to root anyway.
	if (const auto user = launch_url_internal::get_original_user()) {
		return launch_url_internal::launch_url_as_user(user.value(), link);
	}

	GError* error = nullptr;
#if GTK_CHECK_VERSION(3, 22, 0)
	bool status = static_cast<bool>(gtk_show_uri_on_window(window, link.c_str(), GDK_CURRENT_TIME, &error));
#else
	GdkScreen* screen = (window ? gtk_window_get_screen(window) : nullptr);
	bool status = static_cast<bool>(gtk_show_uri(screen, link.c_str(), GDK_CURRENT_TIME, &error));
#endif
	std::unique_ptr<GError, decltype(&g_error_free)> uerror(error, &g_error_free);

	if (!status) {
		return std::string("Cannot open URL")
				+ ((error && error->message) ? (std::string(": ") + error->message) : ".");
	}
	return {};
#endif
}




}  // ns





#endif

/// @}
