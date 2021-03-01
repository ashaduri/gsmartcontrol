###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

# 3.12 for CMAKE_PROJECT_VERSION
# 3.13 for interface sources
cmake_minimum_required(VERSION 3.13)

include(${CMAKE_SOURCE_DIR}/version.txt)

project(gsmartcontrol VERSION ${CMAKE_PROJECT_VERSION})


# Provide DATADIR, etc...
include(GNUInstallDirs)

include(src/build_config/compiler_options.cmake)


# Disable RPATH manipulation, we don't have shared libraries
set(CMAKE_SKIP_BUILD_RPATH TRUE)
set(CMAKE_SKIP_INSTALL_RPATH TRUE)


option(APP_BUILD_EXAMPLES "Build examples" ON)


configure_file(gsmartcontrol.in.spec gsmartcontrol.spec)



add_subdirectory(contrib)
add_subdirectory(data)
add_subdirectory(debian.dist)
add_subdirectory(dependencies)
add_subdirectory(src)

include(data/cpack_options.cmake)



# TODO

## NSIS installer support.
## Requires installed NSIS, smartctl-nc.exe, smartctl.exe, update-smart-drivedb.exe.
## Gtkmm and pcre dlls are required if linking against them.
## dos2unix on build machine.
## Execute only with win32 build present.
#
#WIN_ZIP_NAME = $(PACKAGE)-$(VERSION)-@WINDOWS_SUFFIX@
#
#
#win-dist: win-dist-nocleanup win-dist-cleanup
#
#win-dist-nocleanup: win-dist-prepare nsis-dist-nocleanup win-zip-dist-nocleanup
#
#win-dist-cleanup: win-zip-dist-cleanup nsis-dist-cleanup
#	rm -rf win-dist
#
#win-dist-clean: win-dist-cleanup
#	rm -f gsmartcontrol-*.exe gsmartcontrol-*.zip
#
#
#win-dist-prepare-all: nsis-dist-prepare win-zip-dist-prepare
#
#
##	All of GTK+
#GTK_BIN_FILES = gdk-pixbuf-query-loaders.exe \
#	gspawn-win32-helper-console.exe \
#	gspawn-win32-helper.exe \
#	gspawn-win64-helper-console.exe \
#	gspawn-win64-helper.exe \
#	gtk-query-immodules-3.0.exe \
#	gtk-update-icon-cache-3.0.exe \
#	libatk-1.*.dll \
#	libatkmm-1.*.dll \
#	libbz2-*.dll \
#	libcairo-*.dll \
#	libcairomm-1*.dll \
#	libepoxy-*.dll \
#	libffi-*.dll \
#	libexpat-*.dll \
#	libfontconfig-*.dll \
#	libfreetype-*.dll \
#	libgdk-3*.dll \
#	libgdk_pixbuf-2.*.dll \
#	libgdkmm-3.*.dll \
#	libgio-2.*.dll \
#	libgiomm-2.*.dll \
#	libglib-2.*.dll \
#	libglibmm*2.*.dll \
#	libgmodule-2.*.dll \
#	libgobject-2.*.dll \
#	libgraphite*.dll \
#	libgthread-2.*.dll \
#	libgtk-3*.dll \
#	libgtkmm-3*.dll \
#	libharfbuzz*.dll \
#	libiconv*.dll \
#	libintl*.dll \
#	libjasper*.dll \
#	libjpeg*.dll \
#	liblzma*.dll \
#	libpango*-1.*.dll \
#	libpixman*.dll \
#	libpng16-*.dll \
#	libsigc-2.*.dll \
#	libstdc++-*.dll \
#	libtiff-*.dll \
#	libwinpthread-*.dll \
#	libxml2-*.dll \
#	zlib1.dll
#
#
## Don't strip smartctl (it will only change the header), and
## don't strip update-smart-drivedb (it will break its CRC check).
#win-dist-prepare: all
#	$(MKDIR_P) win-dist
#	cp $(top_srcdir)/data/icon_*.png win-dist/
#
#	$(MKDIR_P) win-dist/ui
#	cp $(top_srcdir)/src/ui/* win-dist/ui/
#
#	$(MKDIR_P) win-dist/doc
#	cp data/nsis/distribution.txt win-dist/doc/
#	cp $(top_srcdir)/doc/* win-dist/doc/
#
#	cp src/gsmartcontrol.exe src/gsmartcontrol.exe.manifest win-dist/
#
#	unix2dos win-dist/doc/*.txt
#
#	cp -p "@WINDOWS_SYSROOT@"/bin/drivedb.h win-dist/
#	cp -p "@WINDOWS_SYSROOT@"/bin/smartctl-nc.exe win-dist/
#	cp -p "@WINDOWS_SYSROOT@"/bin/smartctl.exe win-dist/
#	cp -p "@WINDOWS_SYSROOT@"/bin/update-smart-drivedb.exe win-dist/
#
#	for file in $(GTK_BIN_FILES); do for f in "@WINDOWS_SYSROOT@/bin/"$$file; do if test -f "$${f}"; then cp -p "$${f}" win-dist/; fi; done; done
#
##	<prefix>/etc/gtk-3.0/ should contain settings.ini with a win32 theme.
#	$(MKDIR_P) win-dist/etc
#	$(MKDIR_P) win-dist/etc/fonts
#	$(MKDIR_P) win-dist/etc/gtk-3.0
#	cp -p "@WINDOWS_SYSROOT@"/etc/fonts/fonts.conf win-dist/etc/fonts/
##	cp -p "@WINDOWS_SYSROOT@"/etc/gtk-3.0/im-multipress.conf win-dist/etc/gtk-3.0/
#	cp -p "@WINDOWS_SYSROOT@"/etc/gtk-3.0/settings.ini win-dist/etc/gtk-3.0/
#
#	$(MKDIR_P) win-dist/share
#	cp -p -r "@WINDOWS_SYSROOT@"/share/themes win-dist/share
#
##	needed for file chooser
#	$(MKDIR_P) win-dist/share/glib-2.0
#	cp -p -r "@WINDOWS_SYSROOT@"/share/glib-2.0/schemas win-dist/share/glib-2.0
#
## 	$(MKDIR_P) win-dist/share/icons
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/hicolor win-dist/share/icons
#
##	needed for window titlebar (if using client-side decorations),
##	tree sorting indicators, GUI icons.
#	$(MKDIR_P) win-dist/share/icons/Adwaita/16x16/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/16x16/actions/window-close-symbolic.symbolic.png win-dist/share/icons/Adwaita/16x16/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/16x16/actions/window-maximize-symbolic.symbolic.png win-dist/share/icons/Adwaita/16x16/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/16x16/actions/window-minimize-symbolic.symbolic.png win-dist/share/icons/Adwaita/16x16/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/16x16/actions/window-restore-symbolic.symbolic.png win-dist/share/icons/Adwaita/16x16/actions
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/16x16/actions/pan-down-symbolic.symbolic.png win-dist/share/icons/Adwaita/16x16/actions
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/16x16/actions/pan-up-symbolic.symbolic.png win-dist/share/icons/Adwaita/16x16/actions
#	$(MKDIR_P) win-dist/share/icons/Adwaita/16x16/status
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/16x16/status/dialog-information.png win-dist/share/icons/Adwaita/16x16/status
#	$(MKDIR_P) win-dist/share/icons/Adwaita/22x22/status
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/22x22/status/dialog-information.png win-dist/share/icons/Adwaita/22x22/status
#	$(MKDIR_P) win-dist/share/icons/Adwaita/24x24/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/24x24/actions/window-close-symbolic.symbolic.png win-dist/share/icons/Adwaita/24x24/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/24x24/actions/window-maximize-symbolic.symbolic.png win-dist/share/icons/Adwaita/24x24/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/24x24/actions/window-minimize-symbolic.symbolic.png win-dist/share/icons/Adwaita/24x24/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/24x24/actions/window-restore-symbolic.symbolic.png win-dist/share/icons/Adwaita/24x24/actions
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/24x24/actions/pan-down-symbolic.symbolic.png win-dist/share/icons/Adwaita/24x24/actions
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/24x24/actions/pan-up-symbolic.symbolic.png win-dist/share/icons/Adwaita/24x24/actions
#	$(MKDIR_P) win-dist/share/icons/Adwaita/24x24/status
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/24x24/status/dialog-information.png win-dist/share/icons/Adwaita/24x24/status
#	$(MKDIR_P) win-dist/share/icons/Adwaita/32x32/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/32x32/actions/window-close-symbolic.symbolic.png win-dist/share/icons/Adwaita/32x32/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/32x32/actions/window-maximize-symbolic.symbolic.png win-dist/share/icons/Adwaita/32x32/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/32x32/actions/window-minimize-symbolic.symbolic.png win-dist/share/icons/Adwaita/32x32/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/32x32/actions/window-restore-symbolic.symbolic.png win-dist/share/icons/Adwaita/32x32/actions
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/32x32/actions/pan-down-symbolic.symbolic.png win-dist/share/icons/Adwaita/32x32/actions
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/32x32/actions/pan-up-symbolic.symbolic.png win-dist/share/icons/Adwaita/32x32/actions
#	$(MKDIR_P) win-dist/share/icons/Adwaita/32x32/status
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/32x32/status/dialog-information.png win-dist/share/icons/Adwaita/32x32/status
#	$(MKDIR_P) win-dist/share/icons/Adwaita/48x48/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/48x48/actions/window-close-symbolic.symbolic.png win-dist/share/icons/Adwaita/48x48/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/48x48/actions/window-maximize-symbolic.symbolic.png win-dist/share/icons/Adwaita/48x48/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/48x48/actions/window-minimize-symbolic.symbolic.png win-dist/share/icons/Adwaita/48x48/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/48x48/actions/window-restore-symbolic.symbolic.png win-dist/share/icons/Adwaita/48x48/actions
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/48x48/actions/pan-down-symbolic.symbolic.png win-dist/share/icons/Adwaita/48x48/actions
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/48x48/actions/pan-up-symbolic.symbolic.png win-dist/share/icons/Adwaita/48x48/actions
#	$(MKDIR_P) win-dist/share/icons/Adwaita/48x48/status
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/48x48/status/dialog-information.png win-dist/share/icons/Adwaita/48x48/status
#	$(MKDIR_P) win-dist/share/icons/Adwaita/64x64/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/64x64/actions/window-close-symbolic.symbolic.png win-dist/share/icons/Adwaita/64x64/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/64x64/actions/window-maximize-symbolic.symbolic.png win-dist/share/icons/Adwaita/64x64/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/64x64/actions/window-minimize-symbolic.symbolic.png win-dist/share/icons/Adwaita/64x64/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/64x64/actions/window-restore-symbolic.symbolic.png win-dist/share/icons/Adwaita/64x64/actions
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/64x64/actions/pan-down-symbolic.symbolic.png win-dist/share/icons/Adwaita/64x64/actions
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/64x64/actions/pan-up-symbolic.symbolic.png win-dist/share/icons/Adwaita/64x64/actions
## 	$(MKDIR_P) win-dist/share/icons/Adwaita/64x64/status
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/64x64/status/dialog-information.png win-dist/share/icons/Adwaita/64x64/status
#	$(MKDIR_P) win-dist/share/icons/Adwaita/96x96/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/96x96/actions/window-close-symbolic.symbolic.png win-dist/share/icons/Adwaita/96x96/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/96x96/actions/window-maximize-symbolic.symbolic.png win-dist/share/icons/Adwaita/96x96/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/96x96/actions/window-minimize-symbolic.symbolic.png win-dist/share/icons/Adwaita/96x96/actions
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/96x96/actions/window-restore-symbolic.symbolic.png win-dist/share/icons/Adwaita/96x96/actions
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/96x96/actions/pan-down-symbolic.symbolic.png win-dist/share/icons/Adwaita/96x96/actions
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/96x96/actions/pan-up-symbolic.symbolic.png win-dist/share/icons/Adwaita/96x96/actions
## 	$(MKDIR_P) win-dist/share/icons/Adwaita/96x96/status
## 	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/96x96/status/dialog-information.png win-dist/share/icons/Adwaita/96x96/status
#	$(MKDIR_P) win-dist/share/icons/Adwaita/256x256/status
#	cp -p "@WINDOWS_SYSROOT@"/share/icons/Adwaita/256x256/status/dialog-information.png win-dist/share/icons/Adwaita/256x256/status
#
##	other
#	cp -p "@WINDOWS_SYSROOT@"/bin/libgcc_s_*.dll win-dist/
#	cp -p "@WINDOWS_SYSROOT@"/bin/libpcre-1.dll win-dist/
#	cp -p "@WINDOWS_SYSROOT@"/bin/libpcrecpp-0.dll win-dist/
#
#
#nsis-dist-prepare: win-dist-prepare
#	cp -a win-dist nsis-dist
#	cp $(top_srcdir)/data/gsmartcontrol.ico nsis-dist/
#	cp data/nsis/*.nsi $(top_srcdir)/data/nsis/nsi_* nsis-dist/
#	unix2dos nsis-dist/*.nsi
#
#nsis-dist-nocleanup: nsis-dist-prepare
#	cd nsis-dist && @NSIS_EXEC@ gsmartcontrol.nsi
#	mv nsis-dist/$(PACKAGE)-$(VERSION)-@WINDOWS_SUFFIX@.exe .
#
#nsis-dist-cleanup:
#	rm -rf nsis-dist
#
#nsis-dist: nsis-dist-prepare nsis-dist-nocleanup nsis-dist-cleanup
#
#
#win-zip-dist-prepare: win-dist-prepare
#	cp -a win-dist $(WIN_ZIP_NAME)
#
#win-zip-dist-nocleanup: win-zip-dist-prepare
#	zip -K -9r $(WIN_ZIP_NAME).zip $(WIN_ZIP_NAME)
#
#win-zip-dist-cleanup:
#	rm -rf $(WIN_ZIP_NAME)
#
#win-zip-dist: win-zip-dist-prepare win-zip-dist-nocleanup win-zip-dist-cleanup

