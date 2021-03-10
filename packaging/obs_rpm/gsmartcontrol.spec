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
Source0:		http://downloads.sourceforge.net/%{name}/%{version}/%{name}-%{version}.tar.bz2
Source1:     %{name}-rpmlintrc
BuildRoot:	%{_tmppath}/%{name}-%{version}-build
Summary:	Hard Disk Drive and SSD Health Inspection Tool
Group:		Hardware/Other

# For distributions that are not listed here we don't specify any dependencies to avoid errors.

# SUSE / OpenSUSE. SLES also defines the correct suse_version.
%if 0%{?suse_version}
Requires: smartmontools >= 5.43 xterm
Requires: polkit bash
BuildRequires: cmake >= 3.13.0 gcc-c++ libstdc++-devel pcre-devel gtkmm3-devel >= 3.4.0
BuildRequires: update-desktop-files fdupes hicolor-icon-theme polkit
Recommends: xdg-utils
%endif


# Fedora, CentOS, RHEL
%if 0%{?fedora_version} || 0%{?centos_version} || 0%{?rhel_version}
Requires: smartmontools >= 5.43 polkit bash xterm
%if 0%{?centos_version} || 0%{?rhel_version}
# Use cmake from EPEL
BuildRequires: cmake3 >= 3.13.0
%else
BuildRequires: cmake >= 3.13.0
%endif
BuildRequires: gcc-c++ pcre-devel gtkmm30-devel >= 3.4.0
BuildRequires: desktop-file-utils hicolor-icon-theme make
%endif


%description
GSmartControl is a graphical user interface for smartctl, which is a tool for
querying and controlling SMART (Self-Monitoring, Analysis, and Reporting
Technology) data in hard disk and solid-state drives. It allows you to inspect
the drive's SMART data to determine its health, as well as run various tests
on it.


%prep
%setup -q

%build
%cmake \
	-DAPP_COMPILER_ENABLE_WARNINGS=ON
%cmake_build

%install
%cmake_install

%if 0%{?suse_version}
%suse_update_desktop_file -n %{name}
# There are some png file duplicates, hardlink them.
%fdupes -s %{buildroot}%{_prefix}
%endif

%post
%if 0%{?suse_version}
%desktop_database_post
%icon_theme_cache_post
%endif

%postun
%if 0%{?suse_version}
%desktop_database_postun
%icon_theme_cache_postun
%endif


%if 0%{?fedora_version} || 0%{?centos_version} || 0%{?rhel_version}
%check
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop
%endif


%files
%defattr(-,root,root)

%if 0%{?fedora_version} || 0%{?centos_version} || 0%{?rhel_version}
%license LICENSE.txt
%endif

%attr(0755,root,root) %{_bindir}/%{name}-root
%attr(0755,root,root) %{_sbindir}/%{name}

%if 0%{?suse_version} || 0%{?fedora_version} || 0%{?centos_version} || 0%{?rhel_version}

%if 0%{?suse_version}
#%doc %{_defaultdocdir}/%{name}
%doc %{_datadir}/doc/gsmartcontrol
%endif
%if 0%{?fedora_version} || 0%{?centos_version} || 0%{?rhel_version}
%{_pkgdocdir}
%endif

%else
%doc %{_datadir}/doc/gsmartcontrol
%endif

%{_mandir}/man1/*

%{_datadir}/%{name}
%{_datadir}/applications/*.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/pixmaps/%{name}.png
%{_datadir}/pixmaps/%{name}.xpm
%dir %{_datadir}/metainfo
%{_datadir}/metainfo/%{name}.appdata.xml
%{_datadir}/polkit-1/actions/org.%{name}.policy

%changelog
