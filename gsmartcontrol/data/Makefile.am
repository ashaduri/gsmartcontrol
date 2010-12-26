
SUBDIRS = 16 22 24 32 48 64 128 256 nsis

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
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


# Application pixmap (fallback icon for desktop files).
# Same as 48/gsmartcontrol.png.
# Also, xpm for debian menu, 32x32.
pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = 48/gsmartcontrol.png gsmartcontrol.xpm


# Put everything inside the distribution
EXTRA_DIST = $(appdata_DATA) gsmartcontrol.ico gsmartcontrol.xpm


bin_SCRIPTS = gsmartcontrol-root
CLEANFILES = $(bin_SCRIPTS)

