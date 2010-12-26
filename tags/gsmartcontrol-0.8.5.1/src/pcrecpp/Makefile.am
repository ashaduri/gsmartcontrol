AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO
noinst_LIBRARIES = libpcrecpp.a
libpcrecpp_a_SOURCES = pcre_scanner.cpp pcre_stringpiece.cpp pcrecpp.cpp
noinst_HEADERS = pcre_scanner.h pcre_stringpiece.h pcrecpp.h pcrecpp_internal.h \
	pcrecpparg.h

# Distribute original, unpatched files too. Append .orig_dist to prevent conflict
# with debian .orig files.
EXTRA_DIST = pcrecpp.cc.orig_dist
