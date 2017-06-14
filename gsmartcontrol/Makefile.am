
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



# rpm support

src-rpm: dist
	rpmbuild -ts $(distdir).tar.bz2


# NSIS installer support.
# Requires installed NSIS, smartctl-nc.exe, smartctl.exe, update-smart-drivedb.exe.
# Gtkmm dlls and pcre dlls are required if linking against them.
# dos2unix on build machine.
# Execute only with win32 build present.

WIN_ZIP_NAME = $(PACKAGE)-$(VERSION)-@WINDOWS_SUFFIX@


win-dist: win-dist-nocleanup win-dist-cleanup

win-dist-nocleanup: win-dist-prepare nsis-dist-nocleanup win-zip-dist-nocleanup

win-dist-cleanup: win-zip-dist-cleanup nsis-dist-cleanup
	rm -rf win-dist

win-dist-clean: win-dist-cleanup
	rm -f gsmartcontrol-*.exe gsmartcontrol-*.zip


win-dist-prepare-all: nsis-dist-prepare win-zip-dist-prepare


#	All of GTK+
GTK_BIN_FILES = gdk-pixbuf-query-loaders.exe \
	gspawn-win32-helper-console.exe \
	gspawn-win32-helper.exe \
	gtk-query-immodules-3.0.exe \
	gtk-update-icon-cache-3.0.exe \
	libatk-1.*.dll \
	libatkmm-1.*.dll \
	libbz2-*.dll \
	libcairo-*.dll \
	libcairomm-1*.dll \
	libepoxy-*.dll \
	libffi-*.dll \
	libexpat-*.dll \
	libfontconfig-*.dll \
	libfreetype-*.dll \
	libgdk-3*.dll \
	libgdk_pixbuf-2.*.dll \
	libgdkmm-3.*.dll \
	libgio-2.*.dll \
	libgiomm-2.*.dll \
	libglib-2.*.dll \
	libglibmm*2.*.dll \
	libgmodule-2.*.dll \
	libgobject-2.*.dll \
	libgraphite*.dll \
	libgthread-2.*.dll \
	libgtk-3*.dll \
	libgtkmm-3*.dll \
	libharfbuzz*.dll \
	libiconv*.dll \
	libintl*.dll \
	libjasper*.dll \
	libjpeg*.dll \
	liblzma*.dll \
	libpango*-1.*.dll \
	libpixman*.dll \
	libpng16-*.dll \
	libsigc-2.*.dll \
	libstdc++-*.dll \
	libtiff-*.dll \
	libwinpthread-*.dll \
	libxml2-*.dll \
	zlib1.dll


# Don't strip smartctl (it will only change the header), and
# don't strip update-smart-drivedb (it will break its CRC check).
win-dist-prepare: all
	$(MKDIR_P) win-dist
	cp $(top_srcdir)/data/icon_*.png win-dist/

	$(MKDIR_P) win-dist/doc
	cp data/nsis/distribution.txt win-dist/doc/
	cp $(top_srcdir)/AUTHORS.txt $(top_srcdir)/LICENSE_* $(top_srcdir)/README.txt win-dist/doc/
	cp $(top_srcdir)/NEWS win-dist/doc/NEWS.txt

	cp src/gsmartcontrol.exe src/gsmartcontrol.exe.manifest win-dist/

	unix2dos win-dist/doc/*.txt

	cp -p "@WINDOWS_SYSROOT@"/bin/drivedb.h win-dist/
	cp -p "@WINDOWS_SYSROOT@"/bin/smartctl-nc.exe win-dist/
	cp -p "@WINDOWS_SYSROOT@"/bin/smartctl.exe win-dist/
	cp -p "@WINDOWS_SYSROOT@"/bin/update-smart-drivedb.exe win-dist/

	for file in $(GTK_BIN_FILES); do for f in "@WINDOWS_SYSROOT@/bin/"$$file; do if test -f "$${f}"; then cp -p "$${f}" win-dist/; fi; done; done

	$(MKDIR_P) win-dist/etc
	$(MKDIR_P) win-dist/etc/fonts
	cp -p "@WINDOWS_SYSROOT@"/etc/fonts/fonts.conf win-dist/etc/fonts
	cp -p -r "@WINDOWS_SYSROOT@"/etc/gtk-3.0 win-dist/etc

	$(MKDIR_P) win-dist/share
	cp -p -r "@WINDOWS_SYSROOT@"/share/themes win-dist/share

	$(MKDIR_P) win-dist/share/glib-2.0
	cp -p -r "@WINDOWS_SYSROOT@"/share/glib-2.0/schemas win-dist/share/glib-2.0

# 	$(MKDIR_P) win-dist/share/icons
# 	cp -p "@WINDOWS_SYSROOT@"/share/icons/hicolor win-dist/share/icons

	$(MKDIR_P) win-dist/share/icons/Adwaita/16x16/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/16x16/actions/window-close-symbolic.symbolic.png win-dist/share/icons/Adwaita/16x16/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/16x16/actions/window-maximize-symbolic.symbolic.png win-dist/share/icons/Adwaita/16x16/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/16x16/actions/window-minimize-symbolic.symbolic.png win-dist/share/icons/Adwaita/16x16/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/16x16/actions/window-restore-symbolic.symbolic.png win-dist/share/icons/Adwaita/16x16/actions
	$(MKDIR_P) win-dist/share/icons/Adwaita/24x24/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/24x24/actions/window-close-symbolic.symbolic.png win-dist/share/icons/Adwaita/24x24/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/24x24/actions/window-maximize-symbolic.symbolic.png win-dist/share/icons/Adwaita/24x24/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/24x24/actions/window-minimize-symbolic.symbolic.png win-dist/share/icons/Adwaita/24x24/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/24x24/actions/window-restore-symbolic.symbolic.png win-dist/share/icons/Adwaita/24x24/actions
	$(MKDIR_P) win-dist/share/icons/Adwaita/32x32/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/32x32/actions/window-close-symbolic.symbolic.png win-dist/share/icons/Adwaita/32x32/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/32x32/actions/window-maximize-symbolic.symbolic.png win-dist/share/icons/Adwaita/32x32/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/32x32/actions/window-minimize-symbolic.symbolic.png win-dist/share/icons/Adwaita/32x32/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/32x32/actions/window-restore-symbolic.symbolic.png win-dist/share/icons/Adwaita/32x32/actions
	$(MKDIR_P) win-dist/share/icons/Adwaita/48x48/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/48x48/actions/window-close-symbolic.symbolic.png win-dist/share/icons/Adwaita/48x48/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/48x48/actions/window-maximize-symbolic.symbolic.png win-dist/share/icons/Adwaita/48x48/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/48x48/actions/window-minimize-symbolic.symbolic.png win-dist/share/icons/Adwaita/48x48/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/48x48/actions/window-restore-symbolic.symbolic.png win-dist/share/icons/Adwaita/48x48/actions
	$(MKDIR_P) win-dist/share/icons/Adwaita/64x64/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/64x64/actions/window-close-symbolic.symbolic.png win-dist/share/icons/Adwaita/64x64/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/64x64/actions/window-maximize-symbolic.symbolic.png win-dist/share/icons/Adwaita/64x64/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/64x64/actions/window-minimize-symbolic.symbolic.png win-dist/share/icons/Adwaita/64x64/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/64x64/actions/window-restore-symbolic.symbolic.png win-dist/share/icons/Adwaita/64x64/actions
	$(MKDIR_P) win-dist/share/icons/Adwaita/96x96/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/96x96/actions/window-close-symbolic.symbolic.png win-dist/share/icons/Adwaita/96x96/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/96x96/actions/window-maximize-symbolic.symbolic.png win-dist/share/icons/Adwaita/96x96/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/96x96/actions/window-minimize-symbolic.symbolic.png win-dist/share/icons/Adwaita/96x96/actions
	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/96x96/actions/window-restore-symbolic.symbolic.png win-dist/share/icons/Adwaita/96x96/actions

#	other
	cp -p "@WINDOWS_SYSROOT@"/bin/libgcc_s_*.dll win-dist/
	cp -p "@WINDOWS_SYSROOT@"/bin/libpcre-1.dll win-dist/


nsis-dist-prepare: win-dist-prepare
	cp -a win-dist nsis-dist
	cp $(top_srcdir)/data/gsmartcontrol.ico nsis-dist/
	cp data/nsis/*.nsi $(top_srcdir)/data/nsis/nsi_* nsis-dist/
	unix2dos nsis-dist/*.nsi

nsis-dist-nocleanup: nsis-dist-prepare
	cd nsis-dist && @NSIS_EXEC@ gsmartcontrol.nsi
	mv nsis-dist/$(PACKAGE)-$(VERSION)-@WINDOWS_SUFFIX@.exe .

nsis-dist-cleanup:
	rm -rf nsis-dist

nsis-dist: nsis-dist-prepare nsis-dist-nocleanup nsis-dist-cleanup


win-zip-dist-prepare: win-dist-prepare
	cp -a win-dist $(WIN_ZIP_NAME)

win-zip-dist-nocleanup: win-zip-dist-prepare
	zip -K -9r $(WIN_ZIP_NAME).zip $(WIN_ZIP_NAME)

win-zip-dist-cleanup:
	rm -rf $(WIN_ZIP_NAME)

win-zip-dist: win-zip-dist-prepare win-zip-dist-nocleanup win-zip-dist-cleanup




distclean-local: win-dist-clean



