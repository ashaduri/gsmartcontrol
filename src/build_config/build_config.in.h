/******************************************************************************
License: BSD Zero Clause License
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup build_config
/// \weakgroup build_config
/// @{

#ifndef BUILD_CONFIG_H
#define BUILD_CONFIG_H


// Parts of this file are replaced by cmake

#define PACKAGE_NAME "@CMAKE_PROJECT_NAME@"
#define PACKAGE_VERSION "@CMAKE_PROJECT_VERSION@"

#define PACKAGE_PKGDATA_DIR "@CMAKE_INSTALL_DATADIR@/gsmartcontrol"
#define PACKAGE_SYSCONF_DIR "@CMAKE_INSTALL_SYSCONFDIR@"
#define PACKAGE_LOCALE_DIR "@CMAKE_INSTALL_LOCALEDIR@"
#define PACKAGE_DOC_DIR "@CMAKE_INSTALL_DOCDIR@"

#ifdef DEBUG_BUILD
	#define PACKAGE_TOP_SOURCE_DIR "@CMAKE_SOURCE_DIR@"
#endif

#cmakedefine CONFIG_KERNEL_WINDOWS32
#cmakedefine CONFIG_KERNEL_WINDOWS64
#cmakedefine CONFIG_KERNEL_LINUX
#cmakedefine CONFIG_KERNEL_FREEBSD
#cmakedefine CONFIG_KERNEL_OPENBSD
#cmakedefine CONFIG_KERNEL_NETBSD
#cmakedefine CONFIG_KERNEL_DRAGONFLY
#cmakedefine CONFIG_KERNEL_SOLARIS
#cmakedefine CONFIG_KERNEL_DARWIN
#cmakedefine CONFIG_KERNEL_QNX

#if defined CONFIG_KERNEL_WINDOWS32 || defined CONFIG_KERNEL_WINDOWS64
	#define CONFIG_KERNEL_FAMILY_WINDOWS
#endif



#endif

/// @}
