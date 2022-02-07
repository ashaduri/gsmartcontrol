
# GSmartControl

***Hard disk drive and SSD health inspection tool***

[![license: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
![GitHub release (latest SemVer)](https://img.shields.io/github/v/release/ashaduri/gsmartcontrol)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/528f4f7aaf0e446abf7e55d2affc7bec)](https://app.codacy.com/gh/ashaduri/gsmartcontrol?utm_source=github.com&utm_medium=referral&utm_content=ashaduri/gsmartcontrol&utm_campaign=Badge_Grade_Settings)
![Platforms](https://img.shields.io/badge/platforms-linux%20%7C%20windows%20%7C%20macos%20%7C%20*bsd-blue)
[![Packaging status](https://repology.org/badge/tiny-repos/gsmartcontrol.svg?header=software%20distributions%20and%20repositories)](https://repology.org/project/gsmartcontrol/versions)


---

[GSmartControl](https://gsmartcontrol.shaduri.dev)
is a graphical user interface for smartctl (from [smartmontools](https://www.smartmontools.org/)
package), which is a tool for
querying and controlling [SMART](https://en.wikipedia.org/wiki/S.M.A.R.T.)
(Self-Monitoring, Analysis, and Reporting
Technology) data on modern hard disk and solid-state drives. It allows you to
inspect the drive's SMART data to determine its health, as well as run various
tests on it.

GSmartControl supports ATA drives (both PATA and SATA), various USB to
ATA bridges and drives behind some RAID controllers:
* Adaptec (Linux, some models only)
* Areca (Linux, Windows)
* HP CCISS (Linux)
* HP hpsa / hpahcisr (Linux)
* Intel Matrix Storage (CSMI) (Linux, Windows, FreeBSD)
* LSI 3ware (Linux, Windows)
* LSI MegaRAID (Windows)

Note: Smartmontools supports even more RAID Controllers. The drives
behind such controllers can be manually added to GSmartControl using
"Add Device..." functionality or `--add-device` command-line option.
See [Supported RAID Controllers](https://www.smartmontools.org/wiki/Supported_RAID-Controllers).


## Software Requirements

Note: If using the official Windows package, no additional software is required.

### Build Requirements
* [pcre1](https://www.pcre.org)
* [GTK+ 3](https://www.gtk.org), version 3.4 or higher
* [Gtkmm](https://www.gtkmm.org), version 3.4 or higher

### Runtime Requirements
* [smartmontools](https://www.smartmontools.org/)
* xterm (optional)

### Supported Operating Systems
* Linux - All the popular configurations should work.
* FreeBSD - Tested with DesktopBSD / x86.
* NetBSD - Tested with NetBSD / x86.
* OpenBSD - Tested with OpenBSD / x86-64.
* DragonFlyBSD - Code written but no testing has been performed yet. Expected
to work without any issues.
* Windows Vista SP2 (32-bit and 64-bit), Windows 7 SP1, Windows Server 2008,
Windows 8.1, Windows 10. The Windows port uses pd0, pd1, etc...
for physical drives 0, 1, etc... .
* Mac OS X.
* Solaris.
* QNX - Code written but no testing has been performed yet.


## Usage

### Command-Line Options

GSmartControl inherits options from GTK+ and other libraries, so be sure to
run it with `--help` option to get a full list of accepted parameters.
Note: The Windows version may not have a text output at all, so `--help` and
similar arguments won't have any effect.

The most important parameters are:

`-?`, `--help` - Show help options.

`-l`, `--no-locale` - Don't use system locale.

`-V`, `--version` - Display version information.

`--no-scan` - Don't scan devices on startup.

`--no-hide-tabs` - Don't hide non-identity tabs when SMART is disabled. Useful
for debugging.

`--add-virtual <file>` - Load smartctl data from file, creating a virtual drive. You
can specify this option multiple times.

`--add-device <device>::<type>[::<extra_args>]` - Add a device to device list.
This option is useful with `--no-scan` to list certain drives only. You can specify
this option multiple times.
Example:  
`--add-device /dev/sda --add-device /dev/twa0::3ware,2 --add-device
'/dev/sdb::::-T permissive'`.

`-v`, `--verbose` - Enable verbose logging; same as `--verbosity-level 5`.

`-q`, `--quiet` - Disable logging; same as `--verbosity-level 0`.

`-b`, `--verbosity-level` - Set verbosity level \[0-5].



## Copyright and Licensing

GSmartControl is Copyright (C) 2008 - 2022 Alexander Shaduri [ashaduri@gmail.com](mailto:ashaduri@gmail.com) and contributors.

GSmartControl is licensed under the terms of
[GNU General Public License Version 3](https://www.gnu.org/licenses/gpl-3.0.en.html).

This program is free software: you can redistribute it and/or modify it under
the terms of version 3 of the GNU General Public License as published by the
Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE. See the GNU General Public Licenses for more details.

This product includes icons from Crystal Project,
copyright 2006-2007 Everaldo Coelho [www.everaldo.com](https://www.everaldo.com).
Crystal Project icons are licensed under [GNU LGPL](https://www.gnu.org/licenses/lgpl-3.0.en.html).

