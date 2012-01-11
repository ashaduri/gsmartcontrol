/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_LOCALE_TOOLS_H
#define HZ_LOCALE_TOOLS_H

#include "hz_config.h"  // feature macros

#include <string>
#include <locale>
#include <clocale>  // std::setlocale
#include <stdexcept>  // std::runtime_error


namespace hz {


/**
\file
Locale manipulation facilities

Note: The POSIX man page for setlocale (which libstdc++ is based on)
states: "The locale state is common to all threads within a process."
This may have rather serious implications for thread-safety.
However, for glibc, libstdc++ uses the "uselocale" extension, which
sets locale on per-thread basis.
*/


/// Set the C standard library locale, storing the previous one into \c old_locale.
/// \return false on failure
inline bool locale_c_set(const std::string& loc, std::string& old_locale)
{
	// even though undocumented, glibc returns 0 when the locale string is invalid.
	char* old = std::setlocale(LC_ALL, loc.c_str());
	if (old) {
		old_locale = old;
		return true;
	}
	return false;
}



/// Set the C standard library locale.
/// \return false on failure
inline bool locale_c_set(const std::string& loc)
{
	return std::setlocale(LC_ALL, loc.c_str());  // returns 0 if invalid
}



/// Get current C standard library locale.
inline std::string locale_c_get()
{
	return std::setlocale(LC_ALL, 0);
}



/// Set the C++ standard library locale (may also set the C locale depending
/// on implementation), storing the previous one into \c old_locale.
/// \return false on failure (no exception is thrown)
inline bool locale_cpp_set(const std::locale& loc, std::locale& old_locale)
{
#if !(defined DISABLE_EXCEPTIONS && DISABLE_EXCEPTIONS)
	try {
#endif
		// under FreeBSD (at least 6.x) and OSX, this may throw on anything
		// except "C" and "POSIX" with message:
		// "locale::facet::_S_create_c_locale name not valid".
		// Blame inadequate implementations.
		old_locale = std::locale::global(loc);
#if !(defined DISABLE_EXCEPTIONS && DISABLE_EXCEPTIONS)
	}
	catch (const std::runtime_error& e) {
		return false;
	}
#endif
	return true;
}



/// Set the C++ standard library locale (may also set the C locale depending
/// on implementation), storing the previous one's name into \c old_locale.
/// \return false on failure (no exception is thrown)
inline bool locale_cpp_set(const std::locale& loc, std::string& old_locale)
{
#if !(defined DISABLE_EXCEPTIONS && DISABLE_EXCEPTIONS)
	try {
#endif
		old_locale = std::locale::global(loc).name();  // not sure, but name() may not be unique (?)
#if !(defined DISABLE_EXCEPTIONS && DISABLE_EXCEPTIONS)
	}
	catch (const std::runtime_error& e) {
		return false;
	}
#endif
	return true;
}



/// Set the C++ standard library locale (may also set the C locale depending
/// on implementation).
/// \return false on failure (no exception is thrown)
inline bool locale_cpp_set(const std::locale& loc)
{
#if !(defined DISABLE_EXCEPTIONS && DISABLE_EXCEPTIONS)
	try {
#endif
		std::locale::global(loc);
#if !(defined DISABLE_EXCEPTIONS && DISABLE_EXCEPTIONS)
	}
	catch (const std::runtime_error& e) {
		return false;
	}
#endif
	return true;
}



/// Set the C++ standard library locale name (may also set the C locale depending
/// on implementation), storing the previous one into \c old_locale.
/// \return false on failure (no exception is thrown)
inline bool locale_cpp_set(const std::string& loc, std::locale& old_locale)
{
#if !(defined DISABLE_EXCEPTIONS && DISABLE_EXCEPTIONS)
	try {
#endif
		old_locale = std::locale::global(std::locale(loc.c_str()));
#if !(defined DISABLE_EXCEPTIONS && DISABLE_EXCEPTIONS)
	}
	catch (const std::runtime_error& e) {
		return false;
	}
#endif
	return true;
}


/// Set the C++ standard library locale name (may also set the C locale depending
/// on implementation), storing the previous one's name into \c old_locale.
/// \return false on failure (no exception is thrown)
inline bool locale_cpp_set(const std::string& loc, std::string& old_locale)
{
#if !(defined DISABLE_EXCEPTIONS && DISABLE_EXCEPTIONS)
	try {
#endif
		old_locale = std::locale::global(std::locale(loc.c_str())).name();  // not sure, but name() may not be unique (?)
#if !(defined DISABLE_EXCEPTIONS && DISABLE_EXCEPTIONS)
	}
	catch (const std::runtime_error& e) {
		return false;
	}
#endif
	return true;
}


/// Set the C++ standard library locale name (may also set the C locale depending
/// on implementation).
/// \return false on failure (no exception is thrown)
inline bool locale_cpp_set(const std::string& loc)
{
#if !(defined DISABLE_EXCEPTIONS && DISABLE_EXCEPTIONS)
	try {
#endif
		std::locale::global(std::locale(loc.c_str()));
#if !(defined DISABLE_EXCEPTIONS && DISABLE_EXCEPTIONS)
	}
	catch (const std::runtime_error& e) {
		return false;
	}
#endif
	return true;
}



/// Get current C++ standard library locale. This has no definition,
/// use one of the specializations.
template<typename ReturnType>
ReturnType locale_cpp_get();


/// Get current C++ standard library locale as a string.
/// May return "*" if it's not representable as a string.
template<> inline
std::string locale_cpp_get<std::string>()
{
	return std::locale().name();
}


/// Get current C++ standard library locale std::locale object.
template<> inline
std::locale locale_cpp_get<std::locale>()
{
	return std::locale();
}





/// Temporarily change the C standard library locale as long
/// as this object lives.
class ScopedCLocale {
	public:

		/// Change to classic locale (aka "C")
		ScopedCLocale(bool do_change = true) : do_change_(do_change), bad_(false)
		{
			if (do_change_) {  // avoid unnecessary const char* -> string conversion overhead
				bad_ = locale_c_set("C", old_locale_);
			}
		}

		/// Change to user-specified locale loc.
		ScopedCLocale(const std::string& loc, bool do_change = true) : do_change_(do_change), bad_(false)
		{
			if (do_change_) {
				bad_ = locale_c_set(loc, old_locale_);
			}
		}

		/// Change back the locale
		~ScopedCLocale()
		{
			this->restore();
		}

		/// Get the old locale
		std::string old() const
		{
			return old_locale_;
		}

		/// Return true if locale setting was unsuccessful
		bool bad() const
		{
			return bad_;
		}

		/// Restore the locale. Invoked by destructor.
		bool restore()
		{
			if (do_change_ && !bad_) {
				bad_ = locale_c_set(old_locale_);
				do_change_ = false;  // avoid invoking again
			}
			return bad_;
		}


	private:

		std::string old_locale_;  ///< Old locale
		bool do_change_;  ///< Whether we changed something or not
		bool bad_;  ///< If true, there was some error

};



/// Temporarily change the C++ standard library locale, as long as this
/// object lives. This may also set the C locale depending on implementation.
class ScopedCppLocale {
	public:

		/// Change to classic locale (aka "C")
		ScopedCppLocale(bool do_change = true) : do_change_(do_change), bad_(false)
		{
			if (do_change) {
				bad_ = locale_cpp_set(std::locale::classic(), old_locale_);
			}
		}

		/// Change to user-specified locale loc.
		ScopedCppLocale(const std::string& loc, bool do_change = true) : do_change_(do_change), bad_(false)
		{
			if (do_change_) {
				bad_ = locale_cpp_set(loc, old_locale_);
			}
		}

		/// Change to user-specified locale loc.
		ScopedCppLocale(const std::locale& loc, bool do_change = true) : do_change_(do_change), bad_(false)
		{
			if (do_change_) {
				bad_ = locale_cpp_set(loc, old_locale_);
			}
		}

		/// Change the locale back to the old one
		~ScopedCppLocale()
		{
			this->restore();
		}

		/// Get the old locale
		std::locale old() const
		{
			return old_locale_;
		}

		/// Return true if locale setting was unsuccessful
		bool bad() const
		{
			return bad_;
		}

		/// Restore the locale. Invoked by destructor.
		bool restore()
		{
			if (do_change_ && !bad_) {
				bad_ = locale_cpp_set(old_locale_);
				do_change_ = false;  // avoid invoking again
			}
			return bad_;
		}


	private:

		std::locale old_locale_;  ///< The old locale
		bool do_change_;  ///< Whether we changed the locale or not
		bool bad_;  ///< If true, there was an error setting the locale

};





}  // ns




#endif

/// @}
