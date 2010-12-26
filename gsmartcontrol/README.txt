
About GSmartControl

GSmartControl - Hard disk drive health inspection tool.

GSmartControl is a graphical user interface for smartctl (from smartmontools
package, see http://smartmontools.sourceforge.net), which is a tool for
querying and controlling SMART (Self-Monitoring, Analysis, and Reporting
Technology) data on modern hard disk drives. It allows you to inspect the
drive's SMART data to determine its health, as well as run various tests on
it.

Note: Only ATA drives (both PATA and SATA) are supported for now.

http://gsmartcontrol.berlios.de



Features

Automatically report and hilight any abnormal SMART information.

Ability to enable / disable SMART.

Ability to enable / disable Automatic Offline Data Collection - A short
self-check that the drive will perform automatically every four hours with no
impact on performance.

Ability to set global and per-drive options for smartctl.

Display drive identity, capabilities, attributes, error and self-test logs.

Perform SMART self-tests.

Ability to load smartctl output as a "virtual" device, which acts just like a
real (read-only) device.

Works on most smartctl-supported operating systems.

Extensive help information.



What is SMART?

Short answer: SMART is a technology which provides hard disk drives with
methods to predict certain kinds of failures with certain chance of success.

Long answer: read below.

Self-Monitoring, Analysis, and Reporting Technology, or SMART, is a
monitoring system for hard drives to detect and report various indicators of
reliability, in the hope of anticipating failures. It is implemented inside
the drives SMART provides several ways of monitoring hard drive health. It
may provide information about general health, various drive attributes (for
example, number of unreadable sectors), error logs, and so on. It may also
provide ways to instruct the drive to run various self-tests, which may report
valuable information. It may even automatically scan the disk surface in when
the drive is idle, and repair the defects, reallocating the data to more safe
areas.

While having SMART sounds really good, there are some nuances to consider. One
of the commond pitfalls is that it may create a false sense of security. That
is, a perfectly good SMART data is NOT an indication that the drive won't fail
the next minute. The reverse is also true - some drives may function perfectly
even with not-so-good-looking SMART data. However, as studies indicate, given
a large population of drives, some SMART attributes may reliably predict
drive failures within up to two months.

Another common mistake is to assume that the attribute values are the real
physical values, as experienced by the drive. As manufacturers do not
necessarily agree on precise attribute definitions and measurement units, the
exact meaning of the attributes may vary greatly across different drive
models.

At present SMART is implemented individually by manufacturers, and while some
aspects are standardized for compatibility, others are not. In fact, most
manufacturers refer the users to their own health monitoring utilities, and
advice against taking SMART data seriously. Nevertheless, SMART may prove an
effective measure against data loss.

Yet another issue is that quite often the drives have bugs which prevent
correct SMART usage. This is usually due to buggy firmware, or the
manufacturer ignoring the standard. Luckily, smartmontools usually detects
these bugs and works around them.



Software Requirements

You need to have the following software installed:

* pcre - http://www.pcre.org .

* smartmontools - see http://smartmontools.sourceforge.net .

* GTK+, version 2.6 or higher - see http://www.gtk.org .

* Gtkmm, version 2.6 or higher - see http://www.gtkmm.org .

* libglademm, version 2.4 or higher - see http://www.gtkmm.org .
libglademm is not needed when using GTK 2.12 and Gtkmm 2.12.

Note that GTK+ 2.12 and Gtkmm 2.12 are HIGHLY recommended. While earlier
versions may work, they may produce ugly results and buggy behaviour.
Libglademm is not needed when using GTK / Gtkmm 2.12.

Most of these packages are probably already provided by your distribution. For
example, to build this program on OpenSUSE, you just need to install
smartmontools, gtkmm2-devel and pcre-devel - the rest is installed
automatically by the package manager's dependency resolver.

The following operating systems are supported:

* Linux - All the popular configurations should work.

* FreeBSD - Tested with DesktopBSD 1.6 (FreeBSD 6.3) / x86.

* NetBSD - Tested with NetBSD 4.0.1 / x86.

* OpenBSD - Tested with OpenBSD 4.3 / x86-64 / gcc-3.3.5.

* Solaris - Tested with Solaris 10 / x86 / gcc-3.4.3 / blastwave,
Solaris 10 / x86 / sunstudio12 / sunfreeware. OpenSolaris should work but has
not been tested yet. Note that until either smartctl gets ATA support under
Solaris or GSmartControl gets SCSI support, these configurations are
essentially useless.

* DragonFlyBSD - Code written but no testing has been performed yet. Expected
to work without any issues.

* Windows (NT line only) - Only minimal testing has been performed. Expected
to work. The Windows port uses /dev/pd0, /dev/pd1, etc... for physical drives
0, 1, etc... .

* Mac OS X - Code written but no testing has been performed yet.

* QNX - Code written but no testing has been performed yet.



Installation

Short answer: ./configure; make; make install

Long answer: read below.

First, check if you can find a pre-built package for your distribution or
operating system - they usually provide the best integration and the easiest
installation procedure. For Linux, one option is to try the OpenSUSE Build
Service - it provides ready-to-install packages for various popular Linux
distributions (OpenSUSE, Fedora, etc...). See
http://download.opensuse.org/repositories/home:/alex_sh/ .

If you want to compile from source, check that you have all the required
dependencies (see Software Requirements section). Then the usual

./configure; make; make install

will build and install it. Installation usually requires adminstrative
privileges, but you don't need to install the program in order to run it
directly from the compilation directory.



Permission Problems

Short answer: you need to be root (Administrator in Windows).
In X11, use kdesu, gnomesu, sux, xdg-su or similar.

Long answer: read below.

Most operating systems prohibit direct access to hardware to users with
non-administrative privileges. Unfortunately, to access SMART data, smartctl
needs to directly access the hard drive.

The provided X11 desktop and menu icons should show the "Please enter the root
password" dialog boxes, and, after correct information is entered, should run
this program with root privileges. The dialogs should be available in most
commonly used X11 desktop environments.

Another way is to use the included gsmartcontrol_root.sh script, which finds
the available su program and runs gsmartcontrol with it.

Yet another way is to manually invoke the program with kdesu, gnomesu, sux or
similar programs. For example,

kdesu -u root -c gsmartcontrol

will ask for root password and run gsmartcontrol with root privileges. Replace
"kdesu" with "gnomesu" if using Gnome. The "sux" or "xdg-su" commands may also
help, if neither KDE or GNOME are available.

Please don't set the "setuid" flag on smartctl binary. It is considered a
security risk.



Enable SMART Permanently

Specifications say that once you set a SMART-related property, it will be
preserved across reboots. So, when you, say, enable SMART and Automatic
Offline Data Collection, both will stay enabled until you disable them.

However, BIOS, your operating system, your other operating systems (if
present), and various startup programs may affect that. For example, BIOS may
enable SMART each time you start your computer, so if you disabled SMART
previously, it will be re-enabled on reboot.

The easiest way to work around this is to set the desired settings on system
startup. You may use smartctl or smartd to do that. For example, to enable
both SMART and Automatic Offline Data Collection on /dev/sda, one would write
the following to his/her boot.local, rc.local or similar:

smartctl --smart=on --offlineauto=on /dev/sda

For more information, see smartctl and smartd documentation.



Known Issues

Only ATA (both PATA and SATA) disks are supported for now. The main reasons
are:

* We can't support drives which don't work with smartmontools. This affects
drives which don't support SMART or don't export SMART data correctly (e.g.
USB enclosures, RAIDs, etc...).

* Smartctl's output for SCSI drives is completely different compared to ATA.
Also, SCSI drives are rarely found in desktop systems and the servers rarely
have X11 / Gtkmm running, so this is a low priority task.

* I only have ATA drives, so testing would be almost impossible.

Immediate Offline Tests are not supported. I haven't found a way to reliably
monitor them yet.

You may run several tests on different drives in parallel, but you will only
see the progress information of the last executing test. So, if you run tests
on drives A, B and C (in that order), you won't be able to monitor the tests
on A and B until C completes. And you won't be able to see any progress on A
until both B and C complete their tests.

Testing is only supported on drives which correctly report their progress
information in capabilities.

Not all drives support disabling Automatic Offline Data Collection, even if
they report otherwise.

Running on GTK+ / Gtkmm versions earlier than 2.12 may cause visual artifacts,
usability issues (especially with tooltips and icons), instability, etc...

The texts probably contain a lot of grammatical errors, English being my third
language and all.



Reporting Bugs

If it is a SMART or drive-related problem, please try to test it with smartctl
first. Chances are, the problem you're experiencing is not tied to
GSmartControl, but is a drive firmware or smartctl problem. For example, to
see a complete information about your /dev/sda drive, type the following in a
terminal emulator (e.g., xterm, konsole or gnome-terminal):

smartctl -a /dev/sda

If you still think it's a GSmartControl issue, please collect the following
information about your system. Without it, it may be very hard or impossible
to fix the bug.

* Which operating system you use (for example, OpenSUSE Linux 11.0).

* Which version of GTK and Gtkmm you have installed. Finding this out is very
distribution-specific. For example, on OpenSUSE it would be
"rpm -q gtk2 gtkmm2". Some distributions have gtkmm24 instead. You may also
search them in your distribution's graphical package manager, if there is one.

* Execution log from the program, if possible. To obtain it, run the program
with -v option, e.g. (type the following in a terminal emulator or Run
dialog):

gsmartcontrol_root.sh other -v

Perform the steps needed to reproduce the bug, then go to
"Options -> View Execution Log", and click "Save All".

* Detailed description of steps you performed when the bug occurred.

Once you have this information, send an email to me, Alexander Shaduri
<ashaduri 'at' gmail.com>. Note that I may refer you to smartmontools support
if it's a bug in smartmontools and not GSmartControl. Normally, I won't
redirect your support request to them myself, because they may ask questions
which only you have the answers to.

Please read the "License and Copyright" and "Patch Licensing" sections before
sending any patches.



License and Copyright

For license information, see LICENSE_gsmartcontrol.txt file.

You may notice that GSmartControl is not licensed under "GNU GPL version X or
later", but under "GNU GPL version X and Y". I firmly believe that it's unwise
to license a piece of code under non-existant licenses, whatever anyone else
might say. The reason for this is that one simply CANNOT know that, say, in 20
years FSF won't be bought by some corporation who will release GPL version Z
which will completely reverse the reasons GPL was created for.

The removal of "or later" clause somewhat imposes responsibility on the
copyright holders to review every future version of the license once it's
released, and, if deemed acceptable, re-license the code under the new license
(possibly retaining the old licenses). Unfortunately, this is a necessary
inconvenience we will have to deal with.



Patch Licensing

Due to reasons described in "License and Copyright" section, to make it
possible to re-license the code without tracking down all the people who ever
wrote a patch, the copyrights of all minor patches must be assigned to the
central copyright holder of the project. If the patch is major enough (that
is, it forms a significant part of the program source code), the author may
retain the copyright, if he or she chooses to do so. However, unless the
author plans to maintain his part of the source code, he / she is humbly asked
to consider assigning away his / her copyright. A simple "I disclaim all
copyright to this patch" by the author is sufficient. All credits will be
mentioned in product documentation, whatever the size of the patch is.

Please note that centralization of copyright is needed to maintain a
reasonably healthy legal status of the project. Also note that this method is
not unique - FSF and many other organizations require exactly the same thing.

Some contributors may have reasonable doubts about the future status of this
project. Let me assure you that this project will never have more restrictive
license than GPLv2. If, some time in the future, the GPL is somehow
invalidated in court, the project will be re-licensed under similar (in
spirit) license, if possible, or a license less restrictive than GPL (for
example, the three-clause BSD license).


