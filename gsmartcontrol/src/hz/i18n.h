/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_I18N_H
#define HZ_I18N_H

#include "hz_config.h"  // feature macros


/**
\file
Gettext bridge for internationalization. Sort of like gettext.h,
but a lot simpler / lighter.
This file is for internal use in hz. The application is expected
to use gettext.h or similar mechanisms.

Note: If you are using UTF-8 to display messages in your application
but system locale is not UTF-8, then you need to call gettext's
bind_textdomain_codeset(package, "UTF-8");
to enable locale -> UTF-8 conversion for translated messages.
*/


namespace hz {


/// \def ENABLE_NLS
/// Defined to 0 or 1. If 1, enable native language support.
/// Usually NLS can be disabled through the configure --disable-nls option.
#if defined ENABLE_NLS && ENABLE_NLS

	#include <libintl.h>  // gettext functions
	#include <cstddef>  // std::size_t
	#include <cstring>  // std::strlen, std::memcpy


	namespace internal {

		inline const char* i18n_C_helper(const char* msg_with_context, const char* clean_msg)
		{
			const char* res = gettext(msg_with_context);
			return ((res == msg_with_context) ? clean_msg : res);
		}


		inline const char* i18n_R_helper(const char* context, const char* clean_msg)
		{
			std::size_t context_len = std::strlen(context) + 1;
			std::size_t clean_msg_len = std::strlen(clean_msg) + 1;

			char[] msg_with_context = new char[context_len + clean_msg_len];

			std::memcpy(msg_with_context, context, context_len - 1);
			msg_with_context[context_len - 1] = '\004';
			std::memcpy(msg_with_context + context_len, clean_msg, clean_msg_len);

			const char* res = gettext(msg_with_context);
			if (res == msg_with_context) {
				delete[] msg_with_context;
				return clean_msg;
			}
			delete[] msg_with_context;
			return res;
		}

	}


	// ------- Mark and translate

	/// The main gettext function. Marks and translates at runtime.
	/// You need to pass --keyword=HZ__ to xgettext when extracting messages.
	#define HZ__(String) gettext(String)

	/// Same as HZ__(), but specifies a context too, to e.g.
	/// disambiguate two "Open" menu entries as ("File", "Open") and ("Printer", "Open").
	/// You MUST pass --keyword=C_:1c,2 to xgettext when extracting messages.
	#define HZ_C_(Context, String) i18n_C_helper((Context "\004" String), (String))


	// ------- Mark only

	/// The no-op marking of a string for translation.
	/// You MUST pass --keyword=HZ_N_ to xgettext when extracting messages.
	#define HZ_N_(String) (String)

	/// Same as HZ_N_(), but accepts context too.
	/// --keyword=HZ_NC_:1c,2
	#define HZ_NC_(Context, String) (String)


	// ------- Translate only

	/// Translate a dynamic string.
	#define HZ_R_(String) gettext(String)

	/// Same as HZ_R_(), but accepts context too.
	#define HZ_RC_(Context, String) i18n_R_helper((Context), (String))


#else

	#define HZ__(String) (String)
	#define HZ_C_(Context, String) (String)

	#define HZ_N_(String) (String)
	#define HZ_NC_(Context, String) (String)

	#define HZ_R_(String) (String)
	#define HZ_RC_(Context, String) (String)

#endif


}  // ns hz



#endif

/// @}
