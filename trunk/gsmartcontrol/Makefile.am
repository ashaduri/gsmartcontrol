
# Add autoconf.m4 directory to local macro search path
ACLOCAL_AMFLAGS = -I autoconf.m4

SUBDIRS = src data


# These files are actually needed for compilation.
noinst_DATA = AUTHORS.txt LICENSE_gsmartcontrol.txt README.txt


# For the files to be bundled with the distribution, specify them in
# one of the following.

# These will be installed into docdir.
# Some of these files are actually needed for compilation (see src/res/Makefile.am).
dist_doc_DATA = AUTHORS.txt ChangeLog NEWS README.txt \
	LICENSE_boost_1_0.txt \
	LICENSE_bsd-ucb.txt \
	LICENSE_gpl2.txt \
	LICENSE_gpl3.txt \
	LICENSE_gsmartcontrol.txt \
	LICENSE_hz.txt \
	LICENSE_pcrecpp.txt \
	LICENSE_whatever.txt \
	LICENSE_zlib.txt


# Extra files bundled with distribution.
# file2csource.sh is needed for building but not installed.
EXTRA_DIST = COPYING TODO INSTALL configure autogen.sh \
	baqup gsmartcontrol.kdevelop \
	templates \
	file2csource.sh \
	gsmartcontrol.spec



# PCH support

global_macros.h.gch: $(top_srcdir)/src/global_macros.h
	$(CXX) $(CXXFLAGS) $(DEFS) -x "c++-header" $(top_srcdir)/src/global_macros.h -o global_macros.h.gch

gch: global_macros.h.gch

gch-clean:
	rm -f global_macros.h.gch

distclean-local: gch-clean


# rpm support

src-rpm: dist
	rpmbuild -ts $(distdir).tar.bz2


# Don't use this - it does something weird, and gcc errors out with "long long long" errors, etc...
#rpm: dist
#	rpmbuild -ta $(distdir).tar.bz2


