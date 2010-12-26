
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
	LICENSE_bsd-ucb.txt \
	LICENSE_gpl2.txt \
	LICENSE_gpl3.txt \
	LICENSE_gsmartcontrol.txt \
	LICENSE_hz.txt \
	LICENSE_pcrecpp.txt \
	LICENSE_whatever.txt \
	LICENSE_zlib.txt


# nobase_ preserves their directory names.
nobase_dist_doc_DATA = contrib/cron-based_noadmin/README \
	contrib/cron-based_noadmin/cron_gather_smart.sh \
	contrib/cron-based_noadmin/crontab.example \
	contrib/cron-based_noadmin/smartctl_subst.sh


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



# rpm support

src-rpm: dist
	rpmbuild -ts $(distdir).tar.bz2


# Don't use this - it does something weird, and gcc errors out with "long long long" errors, etc...
#rpm: dist
#	rpmbuild -ta $(distdir).tar.bz2



# NSIS installer support.
# Requires wine, installed NSIS, gtkmm dlls, pcre dlls, smartctl-nc.exe, smartctl.exe.
# dos2unix on build machine.
# Execute only with win32 build present.

WIN_ZIP_NAME = $(PACKAGE)-$(VERSION)-win32

win-dist: win-dist-prepare nsis-dist-nocleanup win-zip-dist

win-dist-clean:
	rm -rf gsmartcontrol-*.exe gsmartcontrol-*.zip nsis-dist $(WIN_ZIP_NAME)


win-dist-prepare: all win-dist-clean
	$(MKDIR_P) nsis-dist

	cp data/nsis/gsmartcontrol.nsi $(top_srcdir)/data/nsis/nsi_* nsis-dist/
	cp $(top_srcdir)/data/gsmartcontrol.ico $(top_srcdir)/data/icon_*.png nsis-dist/

	$(MKDIR_P) nsis-dist/doc
	cp data/nsis/distribution.txt nsis-dist/doc/
	cp $(top_srcdir)/AUTHORS.txt $(top_srcdir)/LICENSE_* $(top_srcdir)/README.txt nsis-dist/doc/
	cp $(top_srcdir)/NEWS nsis-dist/doc/NEWS.txt

	unix2dos nsis-dist/*.nsi nsis-dist/doc/*.txt

	cp "@WINDOWS_DLLS_LOCATION@"/libatkmm-*.dll nsis-dist/
	cp "@WINDOWS_DLLS_LOCATION@"/libcairomm-*.dll nsis-dist/
	cp "@WINDOWS_DLLS_LOCATION@"/libgdkmm-*.dll nsis-dist/
	cp "@WINDOWS_DLLS_LOCATION@"/libgiomm-*.dll nsis-dist/
	cp "@WINDOWS_DLLS_LOCATION@"/libglibmm-*.dll nsis-dist/
	cp "@WINDOWS_DLLS_LOCATION@"/libgtkmm-*.dll nsis-dist/
	cp "@WINDOWS_DLLS_LOCATION@"/libpangomm-*.dll nsis-dist/
	cp "@WINDOWS_DLLS_LOCATION@"/libsigc-*.dll nsis-dist/
	cp "@WINDOWS_DLLS_LOCATION@"/pcre3.dll nsis-dist/
	cp "@WINDOWS_DLLS_LOCATION@"/mingwm10.dll nsis-dist/
	cp "@WINDOWS_DLLS_LOCATION@"/smartctl-nc.exe nsis-dist/
	cp "@WINDOWS_DLLS_LOCATION@"/smartctl.exe nsis-dist/

	cp src/gsmartcontrol.exe nsis-dist/

	$(STRIP) nsis-dist/*.exe
	mv nsis-dist/mingwm10.dll nsis-dist/mingwm10.dll_
	$(STRIP) --strip-debug --strip-unneeded nsis-dist/*.dll
	mv nsis-dist/mingwm10.dll_ nsis-dist/mingwm10.dll


nsis-dist: nsis-dist-nocleanup nsis-cleanup

nsis-dist-nocleanup: win-dist-prepare
	cd nsis-dist && @NSIS_EXEC@ gsmartcontrol.nsi
	mv nsis-dist/$(PACKAGE)-$(VERSION).exe .

nsis-cleanup:
	rm -rf nsis-dist


win-zip-dist: win-zip-dist-nocleanup win-zip-dist-cleanup

win-zip-dist-nocleanup: win-dist-prepare
	mv nsis-dist $(WIN_ZIP_NAME)
	rm -rf $(WIN_ZIP_NAME)/gsmartcontrol.ico $(WIN_ZIP_NAME)/nsi_* $(WIN_ZIP_NAME)/*.nsi
	zip -9r $(WIN_ZIP_NAME).zip $(WIN_ZIP_NAME)

win-zip-dist-cleanup:
	rm -rf $(WIN_ZIP_NAME)




distclean-local: gch-clean win-dist-clean



