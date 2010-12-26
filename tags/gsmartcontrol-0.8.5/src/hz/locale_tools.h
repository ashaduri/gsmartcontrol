/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_LOCALE_TOOLS_H
#define HZ_LOCALE_TOOLS_H

#include "hz_config.h"  // feature macros

#include <string>
#include <locale>


namespace hz {


// NOTE: The POSIX man page for setlocale (which libstdc++ is based on)
// states: "The locale state is common to all threads within a process."
// This may have rather serious implications for thread-safety.

// However, for glibc, libstdc++ uses the "uselocale" extension, which
// sets locale on per-thread basis.


// Temporarily change to indicated locale.

class ScopedLocale {

	public:

		// change to classic locale (aka "C")
		ScopedLocale(bool do_change = true) : do_change_(do_change)
		{
			if (do_change_)
				old_locale_ = std::locale::global(std::locale::classic());
		}

		// change to user-specified locale loc.
		ScopedLocale(const std::string& loc, bool do_change = true) : do_change_(do_change)
		{
			if (do_change_)
				old_locale_ = std::locale::global(std::locale(loc.c_str()));
		}

		// change back
		~ScopedLocale()
		{
			if (do_change_)
				std::locale::global(old_locale_);
		}

		// get old locale
		std::locale get_old() const
		{
			return old_locale_;
		}


	private:
		std::locale	old_locale_;
		bool do_change_;

};





}  // ns




#endif
