###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

# This spec file is for openSUSE Build Service.
# Supported distributions: openSUSE, Fedora, CentOS, RHEL.

Name:		gsmartcontrol
Version: 	git
Release:	0
License:	GPL-3.0
Url:		https://gsmartcontrol.shaduri.dev/
Vendor:		Alexander Shaduri <ashaduri@gmail.com>
# Packager:	Alexander Shaduri <ashaduri@gmail.com>
Source:		http://sourceforge.net/projects/gsmartcontrol/files/%{version}/%{name}-%{version}.tar.bz2
BuildRoot:	%{_tmppath}/%{name}-%{version}-build
Summary:	GSmartControl - Hard Disk Drive and SSD Health Inspection Tool
Group:		Hardware/Other

# Empty debug packages cause errors in new RPM. Disable them.
%global debug_package %{nil}


# For distributions that are not listed here we don't specify any dependencies to avoid errors.

# SUSE / OpenSUSE. SLES also defines the correct suse_version.
%if 0%{?suse_version}
Requires: smartmontools >= 5.43, polkit, bash, xterm
BuildRequires: cmake gcc-c++ libstdc++-devel pcre-devel gtkmm3-devel >= 3.4.0 pkgconf-pkg-config
BuildRequires: update-desktop-files
BuildRequires: fdupes
%endif

# Fedora, CentOS, RHEL
%if 0%{?fedora_version} || 0%{?rhel_version} || 0%{?centos_version}
Requires: smartmontools >= 5.43, polkit, bash, xterm
BuildRequires: cmake pkg-config gcc-c++, pcre-devel, gtkmm30-devel >= 3.4.0
%endif


%description
GSmartControl is a graphical user interface for smartctl, which is a tool for
querying and controlling SMART (Self-Monitoring, Analysis, and Reporting
Technology) data in hard disk and solid-state drives. It allows you to inspect
the drive's SMART data to determine its health, as well as run various tests
on it.


%prep
%setup -q
%cmake

%build
%cmake_build

%install
%cmake_install


%if 0%{?suse_version}
%suse_update_desktop_file -n %{name}

# There are some png file duplicates, hardlink them.
%fdupes

# We install icons, so this is needed.
%post
%icon_theme_cache_post

%postun
%icon_theme_cache_postun

# endif suse
%endif


%clean
rm -rf %buildroot


%files
%defattr(-,root,root)

%attr(0755,root,root) %{_bindir}/gsmartcontrol-root
%attr(0755,root,root) %{_sbindir}/gsmartcontrol

# %%attr(0644,root,root) %%config(noreplace) %%{_sysconfdir}/*

%doc %{_datadir}/doc/gsmartcontrol
%doc %{_mandir}/man1/*

%{_datadir}/gsmartcontrol
# %%{_datadir}/gsmartcontrol/*
%{_datadir}/applications/*.desktop
%{_datadir}/metainfo
%{_datadir}/metainfo/gsmartcontrol.appdata.xml
%{_datadir}/polkit-1
%{_datadir}/polkit-1/actions
%{_datadir}/polkit-1/actions/org.gsmartcontrol.policy
%{_datadir}/icons/*
%{_datadir}/pixmaps/*

%changelog
