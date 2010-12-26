
############################################################################
# Copyright:
#      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
# License: See LICENSE_zlib.txt file
############################################################################


# APP_GET_MT_FLAGS(flags_prefix[, action-if-not-found])
# This macro detects compiler flags which are needed to compile a multi-threaded
# (preferably pthreads-based) applications on target OS. flags_prefixCFLAGS,
# flags_prefixCXXFLAGS and flags_prefixLIBS will be initialized (or appended to)
# with the correct flags.

# You must call AX_COMPILER_VENDOR, APP_DETECT_OS_KERNEL([target], ...)
# and APP_DETECT_OS_ENV([target], ...) before using this macro.

AC_DEFUN([APP_GET_MT_FLAGS], [
	# These requirements check compile-time presence, not that they were called
	AC_REQUIRE([AX_COMPILER_VENDOR])
	AC_REQUIRE([APP_DETECT_OS_KERNEL])
	AC_REQUIRE([APP_DETECT_OS_ENV])

	AC_MSG_CHECKING([for target thread support flags])

	app_cv_target_thread_cflags="";
	app_cv_target_thread_cxxflags="";
	app_cv_target_thread_libs="";
	app_cv_target_thread_found="no";

	# If vendor is not recognized, it's set to "unknown".
	# NOTE: This works only on target compilers!
	if test "x${ax_cv_cxx_compiler_vendor}" = "x"; then
		# msg_result is needed to print the checking... message.
		AC_MSG_RESULT([error])
		AC_MSG_ERROR([[ax_cv_cxx_compiler_vendor] not set.])
	fi
	if test "x${app_cv_target_os_kernel}" = "x"; then
		# msg_result is needed to print the checking... message.
		AC_MSG_RESULT([error])
		AC_MSG_ERROR([[app_cv_target_os_kernel] not set.])
	fi
	if test "x${app_cv_target_os_env}" = "x"; then
		# msg_result is needed to print the checking... message.
		AC_MSG_RESULT([error])
		AC_MSG_ERROR([[app_cv_target_os_env] not set.])
	fi


	# Compilers use at least 3 defines to enable thread-safe support:
	# _THREAD_SAFE, _MT, _REENTRANT.
	# We use the ones that are required for detected platform.

	case "$app_cv_target_os_kernel" in
		linux)
			# glibc manual says that defining either _REENTRANT or _THREAD_SAFE
			# enables some thread-safe implementations of glibc functions.
			if test "$ax_cv_cxx_compiler_vendor" = "gnu" || test "$ax_cv_cxx_compiler_vendor" = "intel" \
					|| test "$ax_cv_cxx_compiler_vendor" = "pathscale"; then
				app_cv_target_thread_cflags="-pthread -D_MT -D_THREAD_SAFE"
				app_cv_target_thread_cxxflags="-pthread -D_MT -D_THREAD_SAFE"
				app_cv_target_thread_libs="-pthread"
				app_cv_target_thread_found="yes";

			elif test "$ax_cv_cxx_compiler_vendor" = "sun"; then
				# suncc's -mt expands to:
				# "-D_REENTRANT -lpthread" on Linux, uses pthreads (pthread.h);
				# "-D_REENTRANT -lthread" on Solaris, uses Solaris threads (thread.h).
				# To use pthreads on Solaris, use "-mt -lpthread".
				# http://docs.sun.com/app/docs/doc/820-7598/bjapp?a=view
				app_cv_target_thread_cflags="-mt"
				app_cv_target_thread_cxxflags="-mt"
				app_cv_target_thread_libs="-mt"
				app_cv_target_thread_found="yes";

			fi
			;;

		windows*)
			# NOTE: Not sure about cygwin.
			# NOTE: This only enables multithreaded code generation. For linking
			# with pthreads-win32 you need additional flags.
			# gcc man page: -mthreads: Support thread-safe exception handling on Mingw32.
			# -mthreads defines -D_MT, and links with -lmingwthrd.
			if test "$ax_cv_cxx_compiler_vendor" = "gnu"; then
				app_cv_target_thread_cflags="-mthreads -D_THREAD_SAFE"
				app_cv_target_thread_cxxflags="-mthreads -D_THREAD_SAFE"
				app_cv_target_thread_libs="-mthreads"
				app_cv_target_thread_found="yes"
			fi
			;;

		interix)
			# Interix kernel is implemented on top of windows kernel.
			# Interix uses -D_REENTRANT for pthreads, automatically enabling -lpthread.
			if test "$ax_cv_cxx_compiler_vendor" = "gnu"; then
				app_cv_target_thread_cflags="-D_REENTRANT"
				app_cv_target_thread_cxxflags="-D_REENTRANT"
				app_cv_target_thread_found="yes"
			fi
			;;

		freebsd | dragonfly)
			# NOTE: Not sure about debian gnu/kfreebsd and gnu/netbsd.
			# Freebsd had -kthread, but it has been discontinued (afaik).
			# For freebsd -pthread replaces libc with libc_r.
			if test "$ax_cv_cxx_compiler_vendor" = "gnu"; then
				app_cv_target_thread_cflags="-pthread -D_MT -D_THREAD_SAFE"
				app_cv_target_thread_cxxflags="-pthread -D_MT -D_THREAD_SAFE"
				app_cv_target_thread_libs="-pthread"
				app_cv_target_thread_found="yes";
			fi
			;;

		openbsd)
			if test "$ax_cv_cxx_compiler_vendor" = "gnu"; then
				app_cv_target_thread_cflags="-pthread -D_REENTRANT"
				app_cv_target_thread_cxxflags="-pthread -D_REENTRANT"
				app_cv_target_thread_libs="-pthread"
				app_cv_target_thread_found="yes";
			fi
			;;

		netbsd)
			# See sys/featuretest.h
			if test "$ax_cv_cxx_compiler_vendor" = "gnu"; then
				app_cv_target_thread_cflags="-pthread -D_REENTRANT"
				app_cv_target_thread_cxxflags="-pthread -D_REENTRANT"
				app_cv_target_thread_libs="-pthread"
				app_cv_target_thread_found="yes";
			fi
			;;

		solaris)
			if test "$ax_cv_cxx_compiler_vendor" = "gnu"; then
				# gnu documentation mentions -threads (and its aliases, -pthreads and -pthread).
				# _REENTRANT is needed to enable some thread-safe equivalents of standard
				# functions.
				# http://docs.sun.com/app/docs/doc/805-8005-04/6j7hcvqs4?l=ko&a=view
				app_cv_target_thread_cflags="-pthreads -D_MT -D_THREAD_SAFE -D_REENTRANT"
				app_cv_target_thread_cxxflags="-pthreads -D_MT -D_THREAD_SAFE -D_REENTRANT"
				app_cv_target_thread_libs="-pthreads"
				app_cv_target_thread_found="yes"

			elif test "$ax_cv_cxx_compiler_vendor" = "sun"; then
				# suncc's -mt expands to:
				# "-D_REENTRANT -lpthread" on Linux, uses pthreads (pthread.h);
				# "-D_REENTRANT -lthread" on Solaris, uses Solaris threads (thread.h).
				# To use pthreads on Solaris, use "-mt -lpthread".
				# http://docs.sun.com/app/docs/doc/820-7598/bjapp?a=view
				# http://docs.sun.com/app/docs/doc/816-5175/pthreads-5?a=view
				# http://docs.sun.com/app/docs/doc/816-5137/compile-74765?a=view
				# We use POSIX threads, so append -lpthread.
				app_cv_target_thread_cflags="-mt -lpthread"
				app_cv_target_thread_cxxflags="-mt -lpthread"
				app_cv_target_thread_libs="-mt -lpthread"
				app_cv_target_thread_found="yes"
			fi
			;;

		darwin)
			if test "$ax_cv_cxx_compiler_vendor" = "gnu"; then
				# Darwin produces mt code by default, and -pthread is an error.
				app_cv_target_thread_found="yes"
			fi
			;;

		qnx)
			if test "$ax_cv_cxx_compiler_vendor" = "gnu"; then
				# afaik, gcc/qnx produces mt code by default (at least on newer qnx releases),
				# so no flags are needed.
				app_cv_target_thread_found="yes"
			fi
			;;

	esac


	if test "$app_cv_target_thread_found" = "no"; then
		AC_MSG_WARN([Cannot detect compiler thread support. Set CFLAGS, CXXFLAGS and LIBS manually.])
		m4_ifvaln([$2],[$2])dnl

	else
		AC_MSG_RESULT([CFLAGS: $app_cv_target_thread_cflags;  CXXFLAGS: $app_cv_target_thread_cxxflags;  LIBS: $app_cv_target_thread_libs])

		if test "x$$1[]CFLAGS" = "x"; then
			$1[]CFLAGS="$app_cv_target_thread_cflags";
		else
			$1[]CFLAGS="$$1[]CFLAGS $app_cv_target_thread_cflags";
		fi
		if test "x$$1[]CXXFLAGS" = "x"; then
			$1[]CXXFLAGS="$app_cv_target_thread_cxxflags";
		else
			$1[]CXXFLAGS="$$1[]CXXFLAGS $app_cv_target_thread_cxxflags";
		fi
		if test "x$$1[]LIBS" = "x"; then
			$1[]LIBS="$app_cv_target_thread_libs";
		else
			$1[]LIBS="$$1[]LIBS $app_cv_target_thread_libs";
		fi
	fi

])





