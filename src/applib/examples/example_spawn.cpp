/******************************************************************************
License: BSD Zero Clause License
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib_examples
/// \weakgroup applib_examples
/// @{

#include <iostream>
#include <glib.h>

#include "hz/main_tools.h"



/// Main function of the test
int main()
{
	return hz::main_exception_wrapper([]()
	{
		GPid pid = {};
		int fd_stdout = 0, fd_stderr = 0;

		const std::string cmd = "iexplore";
	// 	std::vector<std::string> child_argv = Glib::shell_parse_argv(cmd);

		gchar* curr_dir = g_get_current_dir();

		int argcp = 0;  // number of args
		gchar** argvp = nullptr;  // args vector
		GError* shell_error = nullptr;
		g_shell_parse_argv(cmd.c_str(), &argcp, &argvp, &shell_error);

		GError* spawn_error = nullptr;

		g_spawn_async_with_pipes(curr_dir, argvp, nullptr,
				GSpawnFlags(G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD),
				nullptr, nullptr,  // child setup function
				&pid, nullptr, &fd_stdout, &fd_stderr, &spawn_error);

		g_free(curr_dir);

		g_strfreev(argvp);

	#ifdef _WIN32
		GIOChannel* channel_stdout = g_io_channel_win32_new_fd(fd_stdout);
		GIOChannel* channel_stderr = g_io_channel_win32_new_fd(fd_stderr);
	#else
		GIOChannel* channel_stdout = g_io_channel_unix_new(fd_stdout);
		GIOChannel* channel_stderr = g_io_channel_unix_new(fd_stderr);
	#endif

		// blocking writes if the pipe is full helps for small-pipe systems (see man 7 pipe).
		const int channel_flags = ~G_IO_FLAG_NONBLOCK;

		if (channel_stdout) {
			g_io_channel_set_encoding(channel_stdout, nullptr, nullptr);  // binary IO
			g_io_channel_set_flags(channel_stdout, GIOFlags(g_io_channel_get_flags(channel_stdout) & channel_flags), nullptr);
			g_io_channel_set_buffer_size(channel_stdout, 10000);
		}

		if (channel_stderr) {
			g_io_channel_set_encoding(channel_stderr, nullptr, nullptr);  // binary IO
			g_io_channel_set_flags(channel_stderr, GIOFlags(g_io_channel_get_flags(channel_stderr) & channel_flags), nullptr);
			g_io_channel_set_buffer_size(channel_stderr, 10000);
		}

		g_main_loop_run(g_main_loop_new(nullptr, FALSE));

		return EXIT_SUCCESS;
	});
}





/// @}
