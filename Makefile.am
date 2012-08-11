
# Add autoconf.m4 directory to local macro search path
ACLOCAL_AMFLAGS = -I autoconf.m4

SUBDIRS = data debian.dist src


# These files are actually needed for compilation.
noinst_DATA = AUTHORS.txt LICENSE_gsmartcontrol.txt README.txt


# For the files to be bundled with the distribution, specify them in
# one of the following.

# These will be installed into docdir.
# Some of these files are actually needed for compilation (see src/res/Makefile.am).
dist_doc_DATA = AUTHORS.txt ChangeLog NEWS README.txt \
	LICENSE_boost_1_0.txt \
	LICENSE_bsd-ac.txt \
	LICENSE_bsd-ucb.txt \
	LICENSE_gpl2.txt \
	LICENSE_gpl3.txt \
	LICENSE_gsmartcontrol.txt \
	LICENSE_hz.txt \
	LICENSE_pcrecpp.txt \
	LICENSE_unlicense.txt \
	LICENSE_zlib.txt


# nobase_ preserves their directory names.
nobase_dist_doc_DATA = contrib/cron-based_noadmin/README \
	contrib/cron-based_noadmin/cron_gather_smart.sh \
	contrib/cron-based_noadmin/crontab.example \
	contrib/cron-based_noadmin/smartctl_subst.sh


# Extra files bundled with distribution.
# file2csource.sh is needed for building but not installed.
EXTRA_DIST = COPYING INSTALL configure autogen.sh \
	gsmartcontrol.kdev4 \
	file2csource.sh \
	gsmartcontrol.spec



# PCH support

global_macros.h.gch: $(top_srcdir)/src/global_macros.h
	$(CXX) $(CXXFLAGS) $(DEFS) -x "c++-header" $(top_srcdir)/src/global_macros.h -o global_macros.h.gch

gch: global_macros.h.gch

gch-clean:
	rm -f global_macros.h.gch



# rpm support

src-rpm: dist
	rpmbuild -ts $(distdir).tar.bz2


# Don't use this - it does something weird, and gcc errors out with "long long long" errors, etc...
#rpm: dist
#	rpmbuild -ta $(distdir).tar.bz2



# NSIS installer support.
# Requires wine, installed NSIS, smartctl-nc.exe, smartctl.exe, update-smart-drivedb.exe.
# Gtkmm dlls and pcre dlls are required if linking against them.
# dos2unix on build machine.
# Execute only with win32 build present.

WIN_ZIP_NAME = $(PACKAGE)-$(VERSION)-win32
WIN_ZIP_NOGTK_NAME = $(PACKAGE)-$(VERSION)-win32-nogtk


win-dist: win-dist-nocleanup win-dist-cleanup

win-dist-nocleanup: win-dist-prepare nsis-dist-nocleanup win-zip-dist-nocleanup

win-dist-cleanup: win-zip-dist-cleanup nsis-dist-cleanup
	rm -rf win-dist

win-dist-clean: win-dist-cleanup
	rm -f gsmartcontrol-*.exe gsmartcontrol-*.zip


win-dist-prepare-all: nsis-dist-prepare win-zip-dist-prepare


# Don't strip smartctl (it will only change the header), and
# don't strip update-smart-drivedb (it will break its CRC check).
win-dist-prepare: all
	$(MKDIR_P) win-dist
	cp $(top_srcdir)/data/icon_*.png win-dist/

	$(MKDIR_P) win-dist/doc
	cp data/nsis/distribution*.txt win-dist/doc/
	cp $(top_srcdir)/AUTHORS.txt $(top_srcdir)/LICENSE_* $(top_srcdir)/README.txt win-dist/doc/
	cp $(top_srcdir)/NEWS win-dist/doc/NEWS.txt

	cp src/gsmartcontrol.exe src/gsmartcontrol.exe.manifest win-dist/

	unix2dos win-dist/doc/*.txt
	$(STRIP) win-dist/*.exe

	cp -p "@WINDOWS_BINDIR@"/smartctl-nc.exe win-dist/
	cp -p "@WINDOWS_BINDIR@"/smartctl.exe win-dist/
	cp -p "@WINDOWS_BINDIR@"/update-smart-drivedb.exe win-dist/


nsis-dist-prepare: win-dist-prepare
	cp -a win-dist nsis-dist
	cp $(top_srcdir)/data/gsmartcontrol.ico nsis-dist/
	cp data/nsis/*.nsi $(top_srcdir)/data/nsis/nsi_* nsis-dist/
	unix2dos nsis-dist/*.nsi
	cp "@WINDOWS_BINDIR@"/"@GTK_INSTALLER_NAME@".exe nsis-dist/

nsis-dist-nocleanup: nsis-dist-prepare
	cd nsis-dist && @NSIS_EXEC@ gsmartcontrol.nsi
	mv nsis-dist/$(PACKAGE)-$(VERSION).exe .
	cd nsis-dist && @NSIS_EXEC@ /DNO_GTK gsmartcontrol.nsi
	mv nsis-dist/$(PACKAGE)-$(VERSION)-nogtk.exe .

nsis-dist-cleanup:
	rm -rf nsis-dist

nsis-dist: nsis-dist-nocleanup nsis-dist-cleanup


win-zip-dist-prepare:
	cp -a win-dist $(WIN_ZIP_NAME)
	cp -a win-dist $(WIN_ZIP_NOGTK_NAME)
	cp -a "@WINDOWS_BINDIR@"/"@GTK_INSTALLER_NAME@"/* $(WIN_ZIP_NAME)/

win-zip-dist-nocleanup: win-zip-dist-prepare
	zip -K -9r $(WIN_ZIP_NAME).zip $(WIN_ZIP_NAME) -x $(WIN_ZIP_NAME)/doc/distribution-nogtk.txt
	zip -K -9r $(WIN_ZIP_NOGTK_NAME).zip $(WIN_ZIP_NOGTK_NAME) -x $(WIN_ZIP_NOGTK_NAME)/doc/distribution.txt

win-zip-dist-cleanup:
	rm -rf $(WIN_ZIP_NAME)
	rm -rf $(WIN_ZIP_NOGTK_NAME)

win-zip-dist: win-zip-dist-nocleanup win-zip-dist-cleanup




distclean-local: gch-clean win-dist-clean



