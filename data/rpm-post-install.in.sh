###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

# Note: %{_prefix} doesn't work here (always defaults to /usr)

gtk-update-icon-cache-3.0 -f -t "@CMAKE_INSTALL_DATADIR@/icons/hicolor"
