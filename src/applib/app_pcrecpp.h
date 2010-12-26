/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef APP_PCRECPP_H
#define APP_PCRECPP_H

// A wrapper header for bundled pcrecpp

// disable all that declspec BS
#ifndef PCRE_STATIC
#	define PCRE_STATIC
#endif


#include "pcrecpp/pcrecpp_internal.h"
#include "pcrecpp/pcrecpp.h"


#include <string>
#include "hz/debug.h"



inline pcrecpp::RE_Options app_pcre_get_options(const char* modifiers)
{
	// ANYCRLF means any of crlf, cr, lf. Used in ^ and $.
	// This overrides the build-time newline setting of pcre.
#ifdef PCRE_NEWLINE_ANYCRLF
	pcrecpp::RE_Options options(PCRE_NEWLINE_ANYCRLF);
#else
	pcrecpp::RE_Options options;
#endif

	if (modifiers) {
		char c = '\0';
		while ((c = *modifiers++) != '\0') {
			switch (c) {
				// Note: Most of these are from pcretest man page.
				// Perl lacks some of them.
				case 'i': options.set_caseless(true); break;  // case insensitive match.
				case 'm': options.set_multiline(true); break;  // read past the first line too.
				case 's': options.set_dotall(true); break;  // dot matches newlines.
				case 'E': options.set_dollar_endonly(true); break;  // not in perl. php has D. $ matches only at end.
				case 'X': options.set_extra(true); break;  // not in perl. strict escape parsing
				case 'x': options.set_extended(true); break;  // ignore whitespaces
				case '8': options.set_utf8(true); break;  // not in perl. php has u. handles UTF8 chars in pattern.
				case 'U': options.set_ungreedy(true); break;  // not in perl. reverses * and *?
				case 'N': options.set_no_auto_capture(true); break;  // not in perl or php. disables matching parentheses.
				default: debug_out_error("app", DBG_FUNC_MSG << "Unknown modifier \'" << c << "\'\n"); break;
			}
		}
	}

	return options;
}




// Accept pattern in form of "/pattern/modifiers".
// Note: Slashes should be escaped within the pattern.
// If the string doesn't start with a slash, it is treated as an ordinary pattern
// without modifiers.
// This function will make a RE object with ANYCRLF option set for portability
// across various pcre builds.
inline pcrecpp::RE app_pcre_re(const std::string& perl_pattern)
{
	if (perl_pattern.size() >= 2 && perl_pattern[0] == '/') {

		// find the separator
		std::string::size_type endpos = perl_pattern.rfind('/');
		DBG_ASSERT(endpos != std::string::npos);  // shouldn't happen

		// no need to unescape slashes in pattern - pcre seems to not mind.
		return pcrecpp::RE(perl_pattern.substr(1, endpos - 1),
				app_pcre_get_options(perl_pattern.substr(endpos + 1).c_str()));
	}

	return pcrecpp::RE(perl_pattern, app_pcre_get_options(0));
}




inline bool app_pcre_match(const std::string& perl_pattern, const std::string& str,
		const pcrecpp::Arg& ptr1 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr2 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr3 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr4 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr5 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr6 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr7 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr8 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr9 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr10 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr11 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr12 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr13 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr14 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr15 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr16 = pcrecpp::RE::no_arg)
{
	return app_pcre_re(perl_pattern).PartialMatch(str,
			ptr1, ptr2, ptr3, ptr4, ptr5, ptr6, ptr7, ptr8, ptr9, ptr10, ptr11, ptr12, ptr13, ptr14, ptr15, ptr16);
}


// This is needed to avoid confusion with RE.
inline bool app_pcre_match(const char* perl_pattern, const std::string& str,
		const pcrecpp::Arg& ptr1 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr2 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr3 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr4 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr5 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr6 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr7 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr8 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr9 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr10 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr11 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr12 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr13 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr14 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr15 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr16 = pcrecpp::RE::no_arg)
{
	return app_pcre_match(std::string(perl_pattern), str,
			ptr1, ptr2, ptr3, ptr4, ptr5, ptr6, ptr7, ptr8, ptr9, ptr10, ptr11, ptr12, ptr13, ptr14, ptr15, ptr16);
}


inline bool app_pcre_match(const pcrecpp::RE& re, const std::string& str,
		const pcrecpp::Arg& ptr1 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr2 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr3 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr4 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr5 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr6 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr7 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr8 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr9 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr10 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr11 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr12 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr13 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr14 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr15 = pcrecpp::RE::no_arg,
		const pcrecpp::Arg& ptr16 = pcrecpp::RE::no_arg)
{
	return re.PartialMatch(str,
			ptr1, ptr2, ptr3, ptr4, ptr5, ptr6, ptr7, ptr8, ptr9, ptr10, ptr11, ptr12, ptr13, ptr14, ptr15, ptr16);
}




// Replaces every occurence of pattern with replacement in subject.
inline int app_pcre_replace(const std::string& perl_pattern, const std::string& replacement, std::string& subject)
{
	return app_pcre_re(perl_pattern).GlobalReplace(replacement, &subject);
}


// Replaces every occurence of pattern with replacement in subject.
inline int app_pcre_replace(const char* perl_pattern, const std::string& replacement, std::string& subject)
{
	return app_pcre_replace(std::string(perl_pattern), replacement, subject);
}


// Replaces every occurence of pattern with replacement in subject.
inline int app_pcre_replace(const pcrecpp::RE& re, const std::string& replacement, std::string& subject)
{
	return re.GlobalReplace(replacement, &subject);
}




// Replaces first occurence of pattern with replacement in subject.
inline bool app_pcre_replace_once(const std::string& perl_pattern, const std::string& replacement, std::string& subject)
{
	return app_pcre_re(perl_pattern).Replace(replacement, &subject);
}


// Replaces first occurence of pattern with replacement in subject.
inline bool app_pcre_replace_once(const char* perl_pattern, const std::string& replacement, std::string& subject)
{
	return app_pcre_replace_once(std::string(perl_pattern), replacement, subject);
}


// Replaces first occurence of pattern with replacement in subject.
inline bool app_pcre_replace_once(const pcrecpp::RE& re, const std::string& replacement, std::string& subject)
{
	return re.Replace(replacement, &subject);
}



// Escape a string to be used as a regular expression
inline std::string app_pcre_escape(const std::string& str)
{
	return pcrecpp::RE::QuoteMeta(str);
}





#endif
