# GSmartControl

***Hard disk drive and SSD health inspection tool***

---

## Description

[GSmartControl](https://gsmartcontrol.shaduri.dev)
is a graphical user interface for smartctl (from [smartmontools](https://www.smartmontools.org/)
package), which is a tool for
querying and controlling [SMART](https://en.wikipedia.org/wiki/S.M.A.R.T.)
(Self-Monitoring, Analysis, and Reporting
Technology) data on modern hard disk and solid-state drives. It allows you to
inspect the drive's SMART data to determine its health, as well as run various
tests on it.


## Features
- automatically reports and highlights any anomalies;
- allows enabling/disabling SMART;
- supports configuration of global and per-drive options for smartctl;
- performs SMART self-tests;
- displays drive identity information, capabilities, attributes, device statistics, etc...;
- can read in smartctl output from a saved file, interpreting it as a read-only virtual device;
- works on most smartctl-supported operating systems;
- has extensive help information.


## Supported Hardware

GSmartControl supports ATA drives (both PATA and SATA), various USB to
ATA bridges and drives behind some RAID.

See [Smartmontools USB Device Support](http://www.smartmontools.org/wiki/Supported_USB-Devices)
page for an (incomplete) list of supported USB to ATA bridges.

GSmartControl supports the following RAID controllers:
- Adaptec (Linux, some models only)
- Areca (Linux, Windows)
- HP CCISS (Linux)
- HP hpsa / hpahcisr (Linux)
- Intel Matrix Storage (CSMI) (Linux, Windows, FreeBSD)
- LSI 3ware (Linux, Windows)
- LSI MegaRAID (Windows)

Note: Smartmontools supports even
[more RAID Controllers](https://www.smartmontools.org/wiki/Supported_RAID-Controllers).
The drives behind such controllers can be manually entered in GSmartControl using
<tt>Add Device...</tt> functionality or `--add-device` command-line option.


## Downloads

[GSmartControl at Repology](https://repology.org/project/gsmartcontrol/versions)


## Software Requirements

### Supported Operating Systems
* Linux
* Windows Vista SP2 (32-bit and 64-bit) or later. The Windows port uses pd0, pd1, etc...
for physical drives 0, 1, etc... .
* FreeBSD
* NetBSD
* OpenBSD
* DragonFlyBSD
* macOS.
* Solaris.
* QNX - Code written but no testing has been performed yet.

### Build Requirements
* [pcre1](https://www.pcre.org)
* [GTK+ 3](https://www.gtk.org), version 3.4 or higher
* [Gtkmm](https://www.gtkmm.org), version 3.4 or higher

### Runtime Requirements
**Note:** Windows packages already include all the required software. 
* [Smartmontools](https://www.smartmontools.org/)
* xterm (optional, needed to run `update-smart-drivedb`)


## Usage

On Linux and Unix systems, use the desktop menu entry. Alternatively, you can
run `gsmartcontrol-root`, which invokes GSmartControl using your desktop's `su` mechanism.

**Note for macOS:** When installed using Homebrew, GSmartControl can be run by
typing `gsmartcontrol` in Terminal. If all you get is `Command not found`, please run
a `brew doctor` command first.


### Command-Line Arguments

**Note:** The Windows version may not output any text to a command-line window,
so `--help` and similar arguments will be of no help.

The most important arguments are:

`-?`, `--help` - Show help options.

`-V`, `--version` - Display version information.

`--no-scan` - Don't scan devices on startup.

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

#### Advanced Arguments

`-l`, `--no-locale` - Don't use system locale.

`--no-hide-tabs` - Don't hide non-identity tabs when SMART is disabled. Useful
for debugging.



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

