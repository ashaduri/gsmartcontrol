/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_FS_ERROR_HOLDER_H
#define HZ_FS_ERROR_HOLDER_H

#include "hz_config.h"  // feature macros

#include <string>
// cerrno is not directly used here, but will be needed in children.
#include <cerrno>  // errno (not std::errno, it may be a macro)

#if defined ENABLE_GLIB && ENABLE_GLIB
	#include <glib.h>  // g_*
#endif

#include "errno_string.h"  // hz::errno_string
#include "debug.h"  // debug_*
#include "i18n.h"  // HZ__
#include "string_algo.h"  // string_replace


// Filesystem operation error tracking.


namespace hz {



class FsErrorHolder {

	public:

		FsErrorHolder() : error_errno_(0), bad_(false)
		{ }

		virtual ~FsErrorHolder()
		{ }


		// check if the last operation was successful
		bool bad() const
		{
			return bad_;
		}

		// check if the last operation was successful
		bool ok() const
		{
			return !bad_;
		}

		// for easy function result checks
// 		operator bool ()  // cast to bool
// 		{
// 			return !bad();
// 		}

		int get_errno() const
		{
			return error_errno_;
		}



#if defined ENABLE_GLIB && ENABLE_GLIB

		// Get error message in current locale. Useful for outputting to console, etc...
		std::string get_error_locale()
		{
			// Note: Paths are in filesystem charset. On win32 it's always utf-8.
			// Errno string is in libc locale charset or utf8 (if using glib).

			gchar* loc_errno = g_locale_from_utf8(hz::errno_string(error_errno_).c_str(), -1, NULL, NULL, NULL);
			std::string loc_errno_str = (loc_errno ? loc_errno
					: (HZ__("[charset conv error] ") + error_errno_ + hz::errno_string(error_errno_)));
			g_free(loc_errno);

			gchar* p1_utf8 = g_filename_display_name(error_path1_.c_str());  // fs -> utf8. always non-null.
			gchar* p1_locale = g_locale_from_utf8(p1_utf8, -1, NULL, NULL, NULL);  // utf8 -> locale
			std::string p1 = (p1_locale ? p1_locale : (HZ__("[charset conv error] ") + error_path1_));
			g_free(p1_utf8);
			g_free(p1_locale);

			gchar* p2_utf8 = g_filename_display_name(error_path2_.c_str());  // fs -> utf8. always non-null.
			gchar* p2_locale = g_locale_from_utf8(p2_utf8, -1, NULL, NULL, NULL);  // utf8 -> locale
			std::string p2 = (p2_locale ? p2_locale : (HZ__("[charset conv error] ") + error_path2_));
			g_free(p2_utf8);
			g_free(p2_locale);

			std::string msg = error_format_;
			hz::string_replace(msg, "/path1/", p1, 1);
			hz::string_replace(msg, "/path2/", p2, 1);
			hz::string_replace(msg, "/errno/", loc_errno_str, 1);

			return msg;
		}


		// Get error message in UTF-8. Use in GUI messages, etc...
		std::string get_error_utf8()
		{
			// Paths are in filesystem charset, convert to utf8.
// 			gchar* cp1utf8 = g_filename_to_utf8(error_path1_.c_str(), -1, NULL, NULL, NULL);
// 			gchar* cp2utf8 = g_filename_to_utf8(error_path2_.c_str(), -1, NULL, NULL, NULL);

			// unlike g_filename_to_utf8, this always returns something. (available since glib 2.6).
			gchar* p1_utf8 = g_filename_display_name(error_path1_.c_str());
			std::string p1 = (p1_utf8 ? p1_utf8 : (HZ__("[charset conv error] ") + error_path1_));
			g_free(p1_utf8);

			gchar* p2_utf8 = g_filename_display_name(error_path2_.c_str());
			std::string p2 = (p2_utf8 ? p2_utf8 : (HZ__("[charset conv error] ") + error_path2_));
			g_free(p2_utf8);

			std::string msg = error_format_;
			hz::string_replace(msg, "/path1/", p1, 1);
			hz::string_replace(msg, "/path2/", p2, 1);
			hz::string_replace(msg, "/errno/", hz::errno_string(error_errno_), 1);  // Errno string is already utf8 if using glib.

			return msg;
		}


#else  // no glib

		// Without glib, the messages may be really screwed up.
		// We still provide these, but we don't make any guarantees.

		// This _should_ work on windows, but since we're using this function
		// for console and console in windows is screwed up (uses DOS charsets(!)),
		// we can't promise anything.
		std::string get_error_locale()
		{
			std::string msg = error_format_;  // gettext-supplied charset (locale, probably)

		#ifdef _WIN32
			// files are in system locale. we're using console, don't use thread locale.
			char* err_path1 = hz::win32_utf8_to_locale(error_path1_.c_str(), 0, false);
			char* err_path2 = hz::win32_utf8_to_locale(error_path2_.c_str(), 0, false);
			std::string errno_str = hz::errno_string(error_errno_);
			char* errno_cstr = hz::win32_utf8_to_locale(errno_str.c_str(), 0, false);

			hz::string_replace(msg, "/path1/", (err_path1 ? err_path1 : error_path1_), 1);
			hz::string_replace(msg, "/path2/", (err_path2 ? err_path2 : error_path2_), 1);
			hz::string_replace(msg, "/errno/", (errno_cstr ? errno_cstr : errno_str), 1);  // utf-8

			delete[] err_path1;
			delete[] err_path2;
			delete[] errno_cstr;

		#else
			// filesystem charset (possibly locale). screwed up, probably.
			hz::string_replace(msg, "/path1/", error_path1_, 1);
			hz::string_replace(msg, "/path2/", error_path2_, 1);
			hz::string_replace(msg, "/errno/", hz::errno_string(error_errno_), 1);  // locale charset
		#endif

			return msg;
		}


		// This works correctly in windows, since we use only utf-8 paths there.
		std::string get_error_utf8()
		{
			return this->get_error_locale();
		}

#endif



	protected:

		// if errno is not 0, error is appended the string value of errno.
		void set_error(const std::string& error_format, int error_errno = 0,
				const std::string& path1 = "", const std::string path2 = "")
		{
			error_format_ = error_format;
			error_errno_ = error_errno;
			error_path1_ = path1;
			error_path2_ = path2;

			bad_ = true;

			warn();
		}


		void clear_error()
		{
			bad_ = false;
			error_format_.clear();
			error_errno_ = 0;
			error_path1_.clear();
			error_path2_.clear();
		}


		void import_error(const FsErrorHolder& other)
		{
			bad_ = other.bad_;
			error_format_ = other.error_format_;
			error_errno_ = other.error_errno_;
			error_path1_ = other.error_path1_;
			error_path2_ = other.error_path2_;
		}



		virtual void warn()  // override this to e.g. enable GUI notifications
		{
			debug_out_warn("hz", "FS warning: " + this->get_error_locale() + "\n");
		}



		// I thought about making these mutable, but it's a bad solution.
		std::string error_format_;  // Last error message format (gettext-translated).
		std::string error_path1_;  // This replaces <path1> parameter in message format
		std::string error_path2_;  // This replaces <path2> parameter in message format

		int error_errno_;  // if not 0, this holds the last error's errno.

		bool bad_;  // error flag

};




}  // ns hz



#endif
