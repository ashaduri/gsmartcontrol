
About GSmartControl

GSmartControl - Hard disk drive and SSD health inspection tool.

GSmartControl is a graphical user interface for smartctl (from smartmontools
package, see https://www.smartmontools.org/), which is a tool for
querying and controlling SMART (Self-Monitoring, Analysis, and Reporting
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
"Add Device..." functionality or --add-device command-line option.
See https://www.smartmontools.org/wiki/Supported_RAID-Controllers .

https://gsmartcontrol.shaduri.dev



Software Requirements

Note: If using the official Windows package, no additional software is required.

Build requirements:
* pcre 1 - http://www.pcre.org .
* GTK+, version 3.4 or higher - see http://www.gtk.org .
* Gtkmm, version 3.4 or higher - see http://www.gtkmm.org .

Runtime requirements:
* smartmontools - see https://www.smartmontools.org/ .
* xterm

The following operating systems are supported:
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



Command Line Options

GSmartControl inherits options from GTK+ and other libraries, so be sure to
run it with --help option to get a full list of accepted parameters.
Note: The Windows version may not have a text output at all, so --help and
similar arguments won't have any effect.

The most important parameters are:

-?, --help - Show help options.

-l, --no-locale - Don't use system locale.

-V, --version - Display version information.

--no-scan - Don't scan devices on startup.

--no-hide-tabs - Don't hide non-identity tabs when SMART is disabled. Useful
for debugging.

--add-virtual - Load smartctl data from file, creating a virtual drive. You
can specify this option multiple times.

--add-device - Add this device to device list. The format of the device is
"<device>::<type>::<extra_args>", where type and extra_args are optional. This
option is useful with --no-scan to list certain drives only. You can specify
this option multiple times.
Example: --add-device /dev/sda --add-device /dev/twa0::3ware,2 --add-device
'/dev/sdb::::-T permissive'.

-v, --verbose - Enable verbose logging; same as --verbosity-level 5.

-q, --quiet - Disable logging; same as --verbosity-level 0.

-b, --verbosity-level - Set verbosity level [0-5].



License and Copyright

For license information, see LICENSE_gsmartcontrol.txt file.
