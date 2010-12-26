/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_whatever.txt
***************************************************************************/

#include <iostream>
#include <glib.h>



int main(int argc, char** argv)
{
	g_thread_init(NULL);

	GPid pid;
	int fd_stdout = 0, fd_stderr = 0;


	std::string cmd = "iexplore";
// 	std::vector<std::string> child_argv = Glib::shell_parse_argv(cmd);

	gchar* curr_dir = g_get_current_dir();


	int argcp = 0;  // number of args
	gchar** argvp = 0;  // args vector
	GError* shell_error = 0;
	g_shell_parse_argv(cmd.c_str(), &argcp, &argvp, &shell_error);


	GError* spawn_error = 0;

	g_spawn_async_with_pipes(curr_dir, argvp, NULL,
			GSpawnFlags(G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD),
			NULL, NULL,  // child setup function
			&pid, 0, &fd_stdout, &fd_stderr, &spawn_error);

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
	GIOFlags flags = GIOFlags(~G_IO_FLAG_NONBLOCK);

	if (channel_stdout) {
		g_io_channel_set_encoding(channel_stdout, NULL, 0);  // binary IO
		g_io_channel_set_flags(channel_stdout, flags, 0);
		g_io_channel_set_buffer_size(channel_stdout, 10000);
	}

	if (channel_stderr) {
		g_io_channel_set_encoding(channel_stderr, NULL, 0);  // binary IO
		g_io_channel_set_flags(channel_stderr, flags, 0);
		g_io_channel_set_buffer_size(channel_stderr, 10000);
	}



	g_main_loop_run(g_main_loop_new(NULL, false));


	return 0;
}




