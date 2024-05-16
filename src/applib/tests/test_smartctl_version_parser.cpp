/******************************************************************************
License: BSD Zero Clause License
Copyright:
	(C) 2022 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib_tests
/// \weakgroup applib_tests
/// @{

// Catch2 v3
//#include "catch2/catch_test_macros.hpp"

// Catch2 v2
#include "catch2/catch.hpp"

#include "applib/smartctl_version_parser.h"



TEST_CASE("SmartctlVersionParser", "[app][parser]")
{
	std::string version_only, version_full;

	SECTION("Parse with version keyword") {
		SmartctlVersionParser::parse_version_text("smartctl version 5.37", version_only, version_full);
		REQUIRE(version_only == "5.37");
		REQUIRE(version_full == "5.37");
	}

	SECTION("Parse without version keyword") {
		SmartctlVersionParser::parse_version_text("smartctl 5.39", version_only, version_full);
		REQUIRE(version_only == "5.39");
		REQUIRE(version_full == "5.39");
	}

	SECTION("Parse with date (CVS)") {
		SmartctlVersionParser::parse_version_text("smartctl 5.39 2009-06-03 20:10", version_only, version_full);
		REQUIRE(version_only == "5.39");
		REQUIRE(version_full == "5.39 2009-06-03 20:10");
	}

	SECTION("Parse with date (SVN)") {
		SmartctlVersionParser::parse_version_text("smartctl 5.39 2009-08-08 r2873", version_only, version_full);
		REQUIRE(version_only == "5.39");
		REQUIRE(version_full == "5.39 2009-08-08 r2873");
	}

	SECTION("Parse pre-releases") {
		SmartctlVersionParser::parse_version_text("smartctl pre-7.4 2023-06-13 r5481", version_only, version_full);
		REQUIRE(version_only == "7.4");
		REQUIRE(version_full == "pre-7.4 2023-06-13 r5481");
	}

	SECTION("Parse old 5.0") {
		SmartctlVersionParser::parse_version_text("smartctl version 5.0-49", version_only, version_full);
		REQUIRE(version_only == "5.0-49");
		REQUIRE(version_full == "5.0-49");
	}

	SECTION("Parse full output (SVN)") {
		const std::string output =
R"(smartctl 7.2 2020-12-30 r5155 [x86_64-linux-5.3.18-lp152.66-default] (SUSE RPM)
Copyright (C) 2002-20, Bruce Allen, Christian Franke, www.smartmontools.org

smartctl comes with ABSOLUTELY NO WARRANTY. This is free
software, and you are welcome to redistribute it under
the terms of the GNU General Public License; either
version 2, or (at your option) any later version.
See http://www.gnu.org for further details.

smartmontools release 7.2 dated 2020-12-30 at 16:48:30 UTC
smartmontools SVN rev 5155 dated 2020-12-30 at 16:49:18
smartmontools build host: x86_64-suse-linux-gnu
smartmontools build with: C++14, GCC 7.5.0
smartmontools configure arguments: '--host=x86_64-suse-linux-gnu' '--build=x86_64-suse-linux-gnu' '--program-prefix=' '--prefix=/usr' '--exec-prefix=/usr' '--bindir=/usr/bin' '--sbindir=/usr/sbin' '--sysconfdir=/etc' '--datadir=/usr/share' '--includedir=/usr/include' '--libdir=/usr/lib64' '--libexecdir=/usr/lib' '--localstatedir=/var' '--sharedstatedir=/var/lib' '--mandir=/usr/share/man' '--infodir=/usr/share/info' '--disable-dependency-tracking' '--docdir=/usr/share/doc/packages/smartmontools' '--with-selinux' '--with-libsystemd' '--with-systemdsystemunitdir=/usr/lib/systemd/system' '--with-savestates' '--with-attributelog' '--with-nvme-devicescan' 'build_alias=x86_64-suse-linux-gnu' 'host_alias=x86_64-suse-linux-gnu' 'CXXFLAGS=-O2 -g -m64 -fmessage-length=0 -D_FORTIFY_SOURCE=2 -fstack-protector -funwind-tables -fasynchronous-unwind-tables -fPIE ' 'LDFLAGS=-pie' 'CFLAGS=-O2 -g -m64 -fmessage-length=0 -D_FORTIFY_SOURCE=2 -fstack-protector -funwind-tables -fasynchronous-unwind-tables  -fPIE' 'PKG_CONFIG_PATH=:/usr/lib64/pkgconfig:/usr/share/pkgconfig'
)";
		SmartctlVersionParser::parse_version_text(output, version_only, version_full);
		REQUIRE(version_only == "7.2");
		REQUIRE(version_full == "7.2 2020-12-30 r5155");
	}

	SECTION("Parse full output (git)") {
		const std::string output =
R"(smartctl 7.3 (build date Feb 11 2022) [x86_64-linux-5.3.18-lp152.66-default] (local build)
Copyright (C) 2002-22, Bruce Allen, Christian Franke, www.smartmontools.org

smartctl comes with ABSOLUTELY NO WARRANTY. This is free
software, and you are welcome to redistribute it under
the terms of the GNU General Public License; either
version 2, or (at your option) any later version.
See https://www.gnu.org for further details.

smartmontools release 7.3 dated 2020-12-30 at 16:48:30 UTC
smartmontools SVN rev is unknown
smartmontools build host: x86_64-pc-linux-gnu
smartmontools build with: C++11, GCC 7.5.0
smartmontools configure arguments: [no arguments given]

)";
		SmartctlVersionParser::parse_version_text(output, version_only, version_full);
		REQUIRE(version_only == "7.3");
		REQUIRE(version_full == "7.3");
	}

}



/// @}
