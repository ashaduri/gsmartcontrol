
# GSmartControl

***Hard disk drive and SSD health inspection tool***

[![Generic badge](https://img.shields.io/badge/Homepage-gsmartcontrol.shaduri.dev-brightgreen.svg)](https://gsmartcontrol.shaduri.dev)
[![GitHub release (latest SemVer)](https://img.shields.io/github/v/release/ashaduri/gsmartcontrol?label=Version)](https://gsmartcontrol.shaduri.dev/downloads)
[![license: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Platforms](https://img.shields.io/badge/Platforms-linux%20%7C%20windows%20%7C%20macos%20%7C%20*bsd-blue)](https://gsmartcontrol.shaduri.dev/software-requirements)
[![Packaging status](https://repology.org/badge/tiny-repos/gsmartcontrol.svg?header=Software%20distributions%20and%20repositories)](https://repology.org/project/gsmartcontrol/versions)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/528f4f7aaf0e446abf7e55d2affc7bec)](https://app.codacy.com/gh/ashaduri/gsmartcontrol?utm_source=github.com&utm_medium=referral&utm_content=ashaduri/gsmartcontrol&utm_campaign=Badge_Grade_Settings)

---

[GSmartControl](https://gsmartcontrol.shaduri.dev)
is a graphical user interface for smartctl (from [smartmontools](https://www.smartmontools.org/)
package), which is a tool for
querying and controlling [SMART](https://en.wikipedia.org/wiki/Self-Monitoring,_Analysis_and_Reporting_Technology)
(Self-Monitoring, Analysis, and Reporting
Technology) data on modern hard disk and solid-state drives. It allows you to
inspect the drive's SMART data to determine its health, as well as run various
tests on it.


## Downloads

The [Downloads](https://gsmartcontrol.shaduri.dev/downloads) page contains
all the available packages of GSmartControl.


## Features
- automatically reports and highlights any anomalies;
- allows enabling/disabling SMART;
- supports configuration of global and per-drive options for smartctl;
- performs SMART self-tests;
- displays drive identity information, capabilities, attributes, device statistics, etc.;
- can read in smartctl output from a saved file, interpreting it as a read-only virtual device;
- works on most smartctl-supported operating systems;
- has extensive help information.


### Supported Hardware

GSmartControl supports SATA, PATA, and NVMe drives, as well as drives
behind some USB bridges and RAID controllers.
Please see the
[Supported Hardware](https://gsmartcontrol.shaduri.dev/supported-hardware) page
for more information.


### Supported Platforms

GSmartControl supports all major desktop operating systems, including
Linux, Windows, macOS, FreeBSD, and other BSD-style operating systems.
Please see the
[Software Requirements](https://gsmartcontrol.shaduri.dev/software-requirements) page
for more information.


## Copyright and Licensing

GSmartControl is Copyright (C) 2008 - 2024 Alexander Shaduri [ashaduri@gmail.com](mailto:ashaduri@gmail.com) and contributors.

GSmartControl is licensed under the terms of
[GNU General Public License Version 3](https://www.gnu.org/licenses/gpl-3.0.en.html).

This program is free software: you can redistribute it and/or modify it under
the terms of version 3 of the GNU General Public License as published by the
Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE. See the GNU General Public Licenses for more details.

This product includes icons from Oxygen Icons copyright [KDE](https://kde.org)
and licenced under the [GNU LGPL version 3](https://www.gnu.org/licenses/lgpl-3.0.en.html) or later.
