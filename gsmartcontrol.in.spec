
# This spec file is for openSUSE Build Service.
# Supported distributions: openSUSE, Fedora, CentOS, RHEL.

Name:		gsmartcontrol
Version: 	@CMAKE_PROJECT_VERSION@
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


# For non-specified distributions we don't specify any dependencies to avoid errors.


# SUSE / OpenSUSE. SLES also defines the correct suse_version.
%if 0%{?suse_version}

Requires: smartmontools >= 5.43, polkit, bash, xterm
BuildRequires: gcc-c++, libstdc++-devel, pcre-devel, gtkmm3-devel >= 3.4.0
BuildRequires: update-desktop-files
BuildRequires: fdupes

%endif


# Fedora, CentOS, RHEL
%if 0%{?fedora_version} || 0%{?rhel_version} || 0%{?centos_version}

Requires: smartmontools >= 5.43, polkit, bash, xterm
BuildRequires: gcc-c++, pcre-devel, gtkmm30-devel >= 3.4.0

%endif


%description
GSmartControl is a graphical user interface for smartctl, which is a tool for
querying and controlling SMART (Self-Monitoring, Analysis, and Reporting
Technology) data in hard disk and solid-state drives. It allows you to inspect
the drive's SMART data to determine its health, as well as run various tests
on it.

%prep

%setup -q
%configure


%build
make %{?_smp_mflags}


%install

# %%makeinstall
make DESTDIR=%buildroot install-strip
# Remove the icon cache file "make install" generates, to avoid package conflicts.
rm -f $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/icon-theme.cache


%if 0%{?suse_version}

%suse_update_desktop_file -n %{name}

# There are some png file duplicates, hardlink them.
%fdupes

# We install icons, so this is needed.
%if 0%{?suse_version} >= 1140
%post
%icon_theme_cache_post

%postun
%icon_theme_cache_postun
%endif

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