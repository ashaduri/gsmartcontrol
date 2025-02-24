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
* [GTK+ 3](https://www.gtk.org) library, version 3.4 or higher.
* [Gtkmm](https://www.gtkmm.org) library, version 3.4 or higher.
* C++20 Compiler (GCC 11 or later, Clang/libc++ 17 or later, Apple Clang 15 or later)
* [CMake](https://cmake.org), version 3.14 or higher.

### Building from Source
1. Install the dependencies using your package manager.
2. Clone or extract GSmartControl source code. We assume the directory is named `gsmartcontrol`.
3. Build GSmartControl using the following commands:
```bash
cd gsmartcontrol
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

## Runtime Requirements
**Note:** The Windows packages already include all the required software. 
* [Smartmontools](https://www.smartmontools.org/). Windows users have an option to
install a separate version of smartmontools on their systems, and GSmartControl will automatically use it.
* xterm (optional, needed to run `update-smart-drivedb` on Linux / Unix systems).
