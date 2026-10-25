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
#include <vector>
#include <functional>
#include <gtk/gtk.h>


/// \def HZ_LAUNCH_URL_AS_USER
/// Defined to 0 or 1. If 1, launch_url() opens URLs as the original user when running as root.
/// Not needed on macOS: GTK opens URLs using LaunchServices there, which starts the browser
/// in the user's GUI session even under sudo.
#if !defined(_WIN32) && !defined(__APPLE__)
	#define HZ_LAUNCH_URL_AS_USER 1
#else
	#define HZ_LAUNCH_URL_AS_USER 0
#endif


#ifdef _WIN32
	#include <windows.h>  // seems to be needed by shellapi.h
	#include <shellapi.h>  // ShellExecuteW()
	#include "win32_tools.h"  // hz::win32_utf8_to_utf16
#else
	#include <memory>
#endif

#if HZ_LAUNCH_URL_AS_USER
	#include <algorithm>
	#include <cstddef>  // std::size_t
	#include <fstream>
	#include <locale>
	#include <map>
	#include <optional>
	#include <sstream>
	#include <glib.h>  // g_spawn_async, g_child_watch_add_full
	#include <unistd.h>  // geteuid, getppid, setuid, setgid, setgroups (BSD), sysconf, access
	#include <grp.h>  // setgroups (Linux), getgrouplist
	#include <pwd.h>  // getpwuid, getpwnam
	#include <sys/stat.h>  // stat
	#include <sys/types.h>  // uid_t, gid_t, pid_t
	#include <sys/wait.h>  // WIFEXITED, WEXITSTATUS, ...
	#ifdef __FreeBSD__
		#include <sys/param.h>
		#include <sys/sysctl.h>  // sysctl
		#include <sys/user.h>  // kinfo_proc
	#endif
	#include "debug.h"
	#include "env_tools.h"  // env_get_value
	#include "string_algo.h"  // string_split, string_begins_with
	#include "string_num.h"  // string_is_numeric_nolocale
#endif




namespace hz {


/// A function that receives an error message if opening a URL fails after launch_url() returns.
using LaunchUrlErrorCallback = std::function<void(const std::string& error)>;



#if HZ_LAUNCH_URL_AS_USER

namespace launch_url_internal {


	/// Static variable holder
	struct LaunchUrlStaticHolder {
		/// Environment of the user's desktop session (NAME=VALUE), as passed on the command line.
		static inline std::vector<std::string> passed_user_env;
	};


	/// Variables of the user's desktop session that are needed to open URLs, used when the user's
	/// full environment is not available.
	inline const std::vector<std::string>& get_session_env_var_names()
	{
		static const std::vector<std::string> names = {
			"DISPLAY", "XAUTHORITY", "WAYLAND_DISPLAY", "XDG_RUNTIME_DIR", "DBUS_SESSION_BUS_ADDRESS",
			"XDG_CURRENT_DESKTOP", "XDG_SESSION_TYPE", "XDG_SESSION_DESKTOP", "DESKTOP_SESSION",
			"KDE_FULL_SESSION", "KDE_SESSION_VERSION", "GNOME_DESKTOP_SESSION_ID",
			"XDG_DATA_HOME", "XDG_DATA_DIRS", "XDG_CONFIG_HOME", "XDG_CONFIG_DIRS",
			"PATH", "BROWSER", "LANG", "LANGUAGE", "LC_ALL", "LC_MESSAGES",
		};
		return names;
	}


	/// If xdg-open fails within this time, the error is reported. After that, the exit status
	/// most likely belongs to the browser (xdg-open may wait for it to exit), so it's ignored.
	constexpr gint64 child_error_report_timeout_usec = 10 * G_USEC_PER_SEC;


	/// Exit status of the child process if the privileges cannot be dropped
	constexpr int drop_privileges_failed_status = 125;



	/// Unprivileged user that started the program, when it's running as root
	/// (via pkexec, sudo, run0, doas, etc.).
	struct OriginalUser {
		uid_t uid = 0;
		gid_t gid = 0;
		std::string name;
		std::string home;  ///< Home directory
		std::vector<gid_t> groups;  ///< Supplementary groups
		std::string found_by;  ///< How the user was found (for debug output)
	};


	/// Environment for processes launched as the original user
	struct LaunchEnv {
		std::map<std::string, std::string> vars;
		std::string source;  ///< Where the environment came from (for debug output)
	};


	/// Data for the child watch
	struct ChildWatchData {
		LaunchUrlErrorCallback error_callback;
		gint64 start_time = 0;  ///< Monotonic time when the child was started
		std::string user_name;
	};



	/// Check if \c path is an absolute path to a directory owned by \c uid
	inline bool is_dir_owned_by(const std::string& path, uid_t uid)
	{
		struct stat st = {};
		return !path.empty() && path.front() == '/'
				&& stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode) && st.st_uid == uid;
	}


	/// Check if \c path is an absolute path to a socket owned by \c uid
	inline bool is_socket_owned_by(const std::string& path, uid_t uid)
	{
		struct stat st = {};
		return !path.empty() && path.front() == '/'
				&& stat(path.c_str(), &st) == 0 && S_ISSOCK(st.st_mode) && st.st_uid == uid;
	}



	/// Get information about the original user, if running as root on behalf of another user.
	inline std::optional<OriginalUser> get_original_user()
	{
		if (geteuid() != 0) {
			return std::nullopt;
		}

		const struct passwd* pw = nullptr;
		std::string found_by;

		// pkexec and sudo (as well as run0, which sets SUDO_UID for compatibility) tell us directly.
		for (const char* var : {"PKEXEC_UID", "SUDO_UID"}) {
			std::string uid_str;
			uid_t uid = 0;
			if (env_get_value(var, uid_str) && string_is_numeric_nolocale(uid_str, uid, true, 10) && uid != 0) {
				pw = getpwuid(uid);
				found_by = var;
				break;
			}
		}
		// doas gives us the user name
		if (!pw) {
			std::string name;
			if (env_get_value("DOAS_USER", name) && !name.empty()) {
				pw = getpwnam(name.c_str());
				found_by = "DOAS_USER";
			}
		}
		// Other tools (gnomesu, beesu, su-to-root, ...) may keep XAUTHORITY to allow access to the
		// user's display. The file is owned by the user.
		if (!pw) {
			std::string xauthority;
			struct stat st = {};
			if (env_get_value("XAUTHORITY", xauthority) && !xauthority.empty()
					&& stat(xauthority.c_str(), &st) == 0 && st.st_uid != 0) {
				pw = getpwuid(st.st_uid);
				found_by = "XAUTHORITY owner";
			}
		}
		if (!pw || !pw->pw_name || pw->pw_uid == 0) {
			return std::nullopt;
		}

		OriginalUser user;
		user.uid = pw->pw_uid;
		user.gid = pw->pw_gid;
		user.name = pw->pw_name;
		user.home = (pw->pw_dir ? pw->pw_dir : "/");
		user.found_by = found_by;

		// Supplementary groups, so that root's groups are not inherited.
		// getgrouplist() may or may not report the needed size if the buffer is too small, so grow it.
		int num_groups = 64;
		bool groups_found = false;
		for (int attempt = 0; attempt < 4 && !groups_found; ++attempt) {
			user.groups.resize(static_cast<std::size_t>(num_groups));
			int found_num_groups = num_groups;
			if (getgrouplist(user.name.c_str(), user.gid, user.groups.data(), &found_num_groups) >= 0) {
				user.groups.resize(static_cast<std::size_t>(found_num_groups));
				groups_found = true;
			} else {
				num_groups = std::max(found_num_groups, num_groups * 4);
			}
		}
		if (!groups_found) {  // use the primary group only
			user.groups = {user.gid};
		}
		// setgroups() fails if there are more groups than the system supports
		const long max_groups = sysconf(_SC_NGROUPS_MAX);
		if (max_groups > 0 && user.groups.size() > static_cast<std::size_t>(max_groups)) {
			user.groups.resize(static_cast<std::size_t>(max_groups));
		}

		return user;
	}



	/// Parse a list of NUL-separated NAME=VALUE entries
	inline std::map<std::string, std::string> parse_env_block(const std::string& block)
	{
		std::map<std::string, std::string> vars;
		std::vector<std::string> entries;
		hz::string_split(block, '\0', entries, true);
		for (const auto& entry : entries) {
			const std::size_t eq_pos = entry.find('=');
			if (eq_pos != std::string::npos && eq_pos > 0) {
				vars.emplace(entry.substr(0, eq_pos), entry.substr(eq_pos + 1));
			}
		}
		return vars;
	}



	/// Process information needed to find the user's process
	struct ProcessInfo {
		pid_t ppid = 0;  ///< Parent process ID
		uid_t ruid = 0;  ///< Real user ID
		uid_t euid = 0;  ///< Effective user ID
	};


#if defined(__linux__)

	/// Get information about a process
	inline std::optional<ProcessInfo> get_process_info(pid_t pid)
	{
		std::ifstream file("/proc/" + std::to_string(pid) + "/status");
		if (!file) {
			return std::nullopt;
		}
		ProcessInfo info;
		bool ppid_found = false, uid_found = false;
		std::string line;
		while (std::getline(file, line)) {
			std::istringstream iss(line);
			iss.imbue(std::locale::classic());
			std::string key;
			iss >> key;
			if (key == "PPid:") {
				ppid_found = static_cast<bool>(iss >> info.ppid);
			} else if (key == "Uid:") {  // real, effective, saved, filesystem
				uid_found = static_cast<bool>(iss >> info.ruid >> info.euid);
			}
		}
		if (!ppid_found || !uid_found) {
			return std::nullopt;
		}
		return info;
	}


	/// Get the environment of a process, as NUL-separated NAME=VALUE entries
	inline std::string get_process_env_block(pid_t pid)
	{
		std::ifstream file("/proc/" + std::to_string(pid) + "/environ", std::ios::binary);
		if (!file) {
			return {};
		}
		std::ostringstream ss;
		ss << file.rdbuf();
		return ss.str();
	}

#elif defined(__FreeBSD__)

	/// Get information about a process
	inline std::optional<ProcessInfo> get_process_info(pid_t pid)
	{
		int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, static_cast<int>(pid)};
		struct kinfo_proc proc = {};
		std::size_t size = sizeof(proc);
		if (sysctl(mib, 4, &proc, &size, nullptr, 0) != 0 || size != sizeof(proc)) {
			return std::nullopt;
		}
		ProcessInfo info;
		info.ppid = proc.ki_ppid;
		info.ruid = proc.ki_ruid;
		info.euid = proc.ki_uid;
		return info;
	}


	/// Get the environment of a process, as NUL-separated NAME=VALUE entries
	inline std::string get_process_env_block(pid_t pid)
	{
		int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ENV, static_cast<int>(pid)};
		std::size_t size = 0;
		if (sysctl(mib, 4, nullptr, &size, nullptr, 0) != 0 || size == 0) {
			return {};
		}
		std::string block(size, '\0');
		if (sysctl(mib, 4, block.data(), &size, nullptr, 0) != 0) {
			return {};
		}
		block.resize(size);
		return block;
	}

#endif


	/// Get the environment of the closest ancestor process that belongs to the user.
	/// When running via pkexec or sudo, this is the user's process that started it, which has the
	/// user's full desktop session environment (most of it is removed by pkexec and sudo).
	inline std::optional<LaunchEnv> get_ancestor_env([[maybe_unused]] uid_t uid)
	{
#if defined(__linux__) || defined(__FreeBSD__)
		pid_t pid = getppid();
		for (int depth = 0; depth < 32 && pid > 1; ++depth) {
			const auto info = get_process_info(pid);
			if (!info.has_value()) {
				break;
			}
			if (info->ruid == uid && info->euid == uid) {
				const std::string block = get_process_env_block(pid);
				if (!block.empty()) {
					return LaunchEnv{parse_env_block(block), "process " + std::to_string(pid)};
				}
			}
			pid = info->ppid;
		}
#endif
		return std::nullopt;
	}



	/// Prepare the environment for processes launched as the user. Don't pass root's environment as is.
	inline LaunchEnv get_launch_env(const OriginalUser& user)
	{
		const auto& session_var_names = get_session_env_var_names();

		LaunchEnv env;
		if (auto ancestor_env = get_ancestor_env(user.uid)) {
			env = std::move(ancestor_env.value());

		} else {
			env.source = "inherited variables";
			for (const auto& name : session_var_names) {
				std::string value;
				if (name != "PATH" && env_get_value(name, value)) {  // root's PATH is handled below
					env.vars[name] = value;
				}
			}
			// Passed by gsmartcontrol-root. These come from the user's session, so prefer them.
			if (!LaunchUrlStaticHolder::passed_user_env.empty()) {
				env.source += " and command line";
			}
			for (const auto& entry : LaunchUrlStaticHolder::passed_user_env) {
				const std::size_t eq_pos = entry.find('=');
				if (eq_pos == std::string::npos) {
					continue;
				}
				const std::string name = entry.substr(0, eq_pos);
				if (std::find(session_var_names.begin(), session_var_names.end(), name) != session_var_names.end()) {
					env.vars[name] = entry.substr(eq_pos + 1);
				}
			}
		}

		env.vars["HOME"] = user.home;
		env.vars["USER"] = user.name;
		env.vars["LOGNAME"] = user.name;

		// The user's runtime directory is needed for Wayland and the session bus. Use the given
		// one only if it belongs to the user, otherwise fall back to the standard locations
		// (systemd / elogind, FreeBSD pam_xdg).
		if (auto iter = env.vars.find("XDG_RUNTIME_DIR");
				iter != env.vars.end() && !is_dir_owned_by(iter->second, user.uid)) {
			env.vars.erase(iter);
		}
		if (env.vars.find("XDG_RUNTIME_DIR") == env.vars.end()) {
			for (const std::string& dir : {"/run/user/" + std::to_string(user.uid), "/var/run/xdg/" + user.name}) {
				if (is_dir_owned_by(dir, user.uid)) {
					env.vars["XDG_RUNTIME_DIR"] = dir;
					break;
				}
			}
		}

		// The session bus is needed by xdg-open and browsers. If it's a socket, it must belong to the user.
		if (auto iter = env.vars.find("DBUS_SESSION_BUS_ADDRESS"); iter != env.vars.end()) {
			const std::string prefix = "unix:path=";
			if (hz::string_begins_with(iter->second, prefix)) {
				const std::size_t comma_pos = iter->second.find(',');
				const std::string path = iter->second.substr(prefix.size(),
						comma_pos == std::string::npos ? std::string::npos : comma_pos - prefix.size());
				if (!is_socket_owned_by(path, user.uid)) {
					env.vars.erase(iter);
				}
			}
		}
		if (env.vars.find("DBUS_SESSION_BUS_ADDRESS") == env.vars.end()) {
			if (auto iter = env.vars.find("XDG_RUNTIME_DIR"); iter != env.vars.end()) {
				const std::string bus = iter->second + "/bus";
				if (is_socket_owned_by(bus, user.uid)) {
					env.vars["DBUS_SESSION_BUS_ADDRESS"] = "unix:path=" + bus;
				}
			}
		}

		// If the user's PATH is unknown, use the standard directories and the ones where
		// xdg-open and browsers may be found on some systems (Snap, NixOS), as well as root's PATH.
		if (env.vars.find("PATH") == env.vars.end()) {
			std::string path = "/usr/local/bin:/usr/bin:/bin";
			std::string root_path;
			if (env_get_value("PATH", root_path) && !root_path.empty()) {
				path += ":" + root_path;
			}
			path += ":/snap/bin:/run/current-system/sw/bin";
			env.vars["PATH"] = path;
		}

		return env;
	}



	/// Find an executable in a colon-separated list of directories.
	/// \return absolute path to the executable, or an empty string if not found.
	inline std::string find_program_in_path(const std::string& program, const std::string& path)
	{
		std::vector<std::string> dirs;
		hz::string_split(path, ':', dirs, true);
		for (const auto& dir : dirs) {
			if (dir.front() != '/') {  // relative directories are unsafe
				continue;
			}
			const std::string file = dir + "/" + program;
			struct stat st = {};
			if (stat(file.c_str(), &st) == 0 && S_ISREG(st.st_mode) && access(file.c_str(), X_OK) == 0) {
				return file;
			}
		}
		return {};
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
			_exit(drop_privileges_failed_status);
		}
		if (setuid(0) == 0) {  // make sure root cannot be regained
			_exit(drop_privileges_failed_status);
		}
	}



	/// Called (in the main loop) when xdg-open exits. Reports early errors to the callback.
	inline void on_launched_child_exited(GPid pid, gint wait_status, gpointer data)
	{
		const auto* watch_data = static_cast<const ChildWatchData*>(data);
		g_spawn_close_pid(pid);
		const gint64 elapsed_usec = g_get_monotonic_time() - watch_data->start_time;

		std::string error;
		bool privileges_not_dropped = false;
		if (WIFEXITED(wait_status)) {
			const int exit_status = WEXITSTATUS(wait_status);
			if (exit_status == drop_privileges_failed_status) {
				error = "Cannot switch to user \"" + watch_data->user_name + "\".";
				privileges_not_dropped = true;
			} else if (exit_status == 3) {  // xdg-open exit statuses
				error = "xdg-open could not find a required tool.";
			} else if (exit_status == 4) {
				error = "xdg-open could not find an application to open it with.";
			} else if (exit_status != 0) {
				error = "xdg-open exited with status " + std::to_string(exit_status) + ".";
			}
		} else if (WIFSIGNALED(wait_status)) {
			error = "xdg-open was terminated by signal " + std::to_string(WTERMSIG(wait_status)) + ".";
		}

		debug_out_info("hz", DBG_FUNC_MSG << "xdg-open (pid " << pid << ") exited after "
				<< (elapsed_usec / 1000) << " ms: " << (error.empty() ? std::string("success.") : error) << "\n");

		if (error.empty()) {
			return;
		}
		if (elapsed_usec > child_error_report_timeout_usec && !privileges_not_dropped) {
			debug_out_info("hz", DBG_FUNC_MSG << "Not reporting the error, it's probably from the browser exiting.\n");
			return;
		}
		if (watch_data->error_callback) {
			watch_data->error_callback("Cannot open URL: " + error);
		}
	}



	/// Launch the URL with xdg-open as the original user.
	/// \return error message on error, empty string on success.
	inline std::string launch_url_as_user(const OriginalUser& user, const std::string& link,
			LaunchUrlErrorCallback async_error_callback)
	{
		const LaunchEnv env = get_launch_env(user);

		debug_out_info("hz", DBG_FUNC_MSG << "Opening URL as user \"" << user.name << "\" (uid " << user.uid
				<< ", found by " << user.found_by << "), environment from " << env.source << ".\n");
		for (const auto& name : get_session_env_var_names()) {
			if (auto iter = env.vars.find(name); iter != env.vars.end()) {
				debug_out_dump("hz", "\t" << name << "=" << iter->second << "\n");
			}
		}

		const std::string xdg_open = find_program_in_path("xdg-open", env.vars.at("PATH"));
		if (xdg_open.empty()) {
			return "Cannot open URL: xdg-open not found. Please install xdg-utils.";
		}

		std::vector<std::string> args = {xdg_open, link};
		std::vector<gchar*> argv;
		for (auto& arg : args) {
			argv.push_back(arg.data());
		}
		argv.push_back(nullptr);

		std::vector<std::string> env_strings;
		for (const auto& [name, value] : env.vars) {
			env_strings.push_back(name + "=" + value);
		}
		std::vector<gchar*> envp;
		for (auto& var : env_strings) {
			envp.push_back(var.data());
		}
		envp.push_back(nullptr);

		// The child is watched to report xdg-open errors, but we don't wait for it,
		// since xdg-open may not return until the browser is closed.
		// "/" is used as working directory, since root may not have access to the user's home
		// directory (e.g. on NFS).
		GPid pid = 0;
		GError* error = nullptr;
		const bool status = static_cast<bool>(g_spawn_async("/", argv.data(), envp.data(),
				GSpawnFlags(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_STDOUT_TO_DEV_NULL),
				&drop_privileges_child_setup, const_cast<OriginalUser*>(&user), &pid, &error));
		std::unique_ptr<GError, decltype(&g_error_free)> uerror(error, &g_error_free);

		if (!status) {
			return std::string("Cannot open URL")
					+ ((error && error->message) ? (std::string(": ") + error->message) : ".");
		}

		debug_out_info("hz", DBG_FUNC_MSG << "Started " << xdg_open << " (pid " << pid << ").\n");

		auto* watch_data = new ChildWatchData{std::move(async_error_callback), g_get_monotonic_time(), user.name};
		g_child_watch_add_full(G_PRIORITY_DEFAULT, pid, &on_launched_child_exited, watch_data,
				[](gpointer data) { delete static_cast<ChildWatchData*>(data); });

		return {};
	}


}  // ns launch_url_internal

#endif  // HZ_LAUNCH_URL_AS_USER



/// Set the environment of the user's desktop session (NAME=VALUE entries), as passed on the command
/// line by gsmartcontrol-root. When running as root, it's used to open URLs as the original user
/// if the user's environment cannot be obtained otherwise. Unknown variables are ignored.
inline void launch_url_set_user_session_env([[maybe_unused]] std::vector<std::string> env)
{
#if HZ_LAUNCH_URL_AS_USER
	launch_url_internal::LaunchUrlStaticHolder::passed_user_env = std::move(env);
#endif
}



/// Open URL in browser or mailto: link in mail client.
/// Return error message on error, empty string otherwise.
/// When running as root, the URL is opened as the original user, and errors that happen after this
/// function returns are passed to \c async_error_callback (called from the main loop).
/// The link is in utf-8 in windows.
inline std::string launch_url([[maybe_unused]] GtkWindow* window, const std::string& link,
		[[maybe_unused]] LaunchUrlErrorCallback async_error_callback = nullptr)
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

#if HZ_LAUNCH_URL_AS_USER
	// When running as root on behalf of a user (e.g. via gsmartcontrol-root), the browser should
	// run as that user: browsers refuse to run as root in a user's session, and the user's
	// desktop session (D-Bus, etc.) is not accessible to root anyway.
	if (const auto user = launch_url_internal::get_original_user()) {
		return launch_url_internal::launch_url_as_user(user.value(), link, std::move(async_error_callback));
	}
#endif

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
