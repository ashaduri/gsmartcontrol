
find_package(PkgConfig REQUIRED)

# Don't make it REQUIRED, we may want to build only the parsers
pkg_check_modules(Gtkmm IMPORTED_TARGET GLOBAL gtkmm-3.0 >= 3.0)

# pcrecpp from pcre1
pkg_check_modules(Pcrecpp REQUIRED IMPORTED_TARGET GLOBAL libpcrecpp)


