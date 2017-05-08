
SUBDIRS = 16 22 24 32 48 64 128 256 nsis


dist_man_MANS = gsmartcontrol.1

install-extra-mans:
	cd $(DESTDIR)$(mandir)/man1 && $(LN_S) -f gsmartcontrol.1 gsmartcontrol-root.1

uninstall-extra-mans:
	cd $(DESTDIR)$(mandir)/man1 && rm -f gsmartcontrol-root.1


# This command is needed if installing with make install (the cache
# file is ignored for rpms).
gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: install-extra-mans update-icon-cache
uninstall-hook: uninstall-extra-mans update-icon-cache

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating GTK icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi


# Application data, installs to /usr/share/appname
appdatadir = $(pkgdatadir)
appdata_DATA = icon_cddvd.png icon_hdd.png


# pkgdatadir = $(datadir)/gsmartcontrol
# pkgdata_DATA =

# Desktop files
desktopdir = $(datadir)/applications
desktop_DATA = gsmartcontrol.desktop

# Appdata files
metainfodir = $(datadir)/metainfo
metainfo_DATA = gsmartcontrol.appdata.xml


# Application pixmap (fallback icon for desktop files).
# Same as 48/gsmartcontrol.png.
# Also, xpm for debian menu, 32x32.
pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = 48/gsmartcontrol.png gsmartcontrol.xpm


# Put everything inside the distribution
EXTRA_DIST = $(appdata_DATA) gsmartcontrol.ico gsmartcontrol.xpm


bin_SCRIPTS = gsmartcontrol-root
CLEANFILES = $(bin_SCRIPTS)

