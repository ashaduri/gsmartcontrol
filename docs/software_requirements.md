# Software Requirements

**Note:** If using the official Windows package, no additional software is required.

## Supported Operating Systems
* Linux
* Windows Vista SP2 (32-bit and 64-bit) or later. The Windows port uses pd0, (pd1, ...)
for physical drives 0, (1, ...).
* FreeBSD
* NetBSD
* OpenBSD
* DragonFlyBSD
* macOS.
* Solaris.
* QNX - Code written but no testing has been performed yet.

## Build Requirements
* [pcre1](https://www.pcre.org).
* [GTK+ 3](https://www.gtk.org), version 3.4 or higher.
* [Gtkmm](https://www.gtkmm.org), version 3.4 or higher.

## Runtime Requirements
**Note:** The Windows packages already include all the required software. 
* [Smartmontools](https://www.smartmontools.org/). Windows users have an option to
install a separate version of smartmontools on their systems, and GSmartControl will automatically use it.
* xterm (optional, needed to run `update-smart-drivedb` on Linux / Unix systems).
