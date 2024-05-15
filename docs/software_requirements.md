---
title: "Software Requirements"
permalink: /software-requirements
---

# Software Requirements

**Note:** If using the official Windows packages, no additional software is required.

## Supported Operating Systems
* Linux
* Windows (Vista SP2 or later)
  * **Note:** The Windows port uses `pd0`, (`pd1`, ...) devices
  for physical drives 0, (1, ...).
* FreeBSD
* NetBSD
* OpenBSD
* DragonFlyBSD
* macOS
* Solaris
* QNX (code written but no testing has been performed yet).

## Build Requirements
**Note:** These are required only if you're building GSmartControl from source code.
* [pcre1](https://www.pcre.org) library.
* [GTK+ 3](https://www.gtk.org) library, version 3.4 or higher.
* [Gtkmm](https://www.gtkmm.org) library, version 3.4 or higher.
* C++20 Compiler (GCC 13 or later, Clang/libc++ 17 or later, Apple Clang 15 or later)

## Runtime Requirements
**Note:** The Windows packages already include all the required software. 
* [Smartmontools](https://www.smartmontools.org/). Windows users have an option to
install a separate version of smartmontools on their systems, and GSmartControl will automatically use it.
* xterm (optional, needed to run `update-smart-drivedb` on Linux / Unix systems).
