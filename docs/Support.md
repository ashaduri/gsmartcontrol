# GSmartControl - Support

## Reporting Bugs

Please report bugs at [GSmartControl's GitHub Page](https://github.com/ashaduri/gsmartcontrol).

If it is a SMART or drive-related problem, please try to test it with smartctl first.
Chances are, the problem you're experiencing is not tied to GSmartControl,
but is a drive firmware or smartctl problem. For example, to see complete
information about your /dev/sda drive, type the following in a terminal
emulator (as root, using sudo or su):
```
smartctl -x /dev/sda
```
**Note:** If using Windows, the device name should be `/dev/pd1` for the
second physical drive, etc... Run cmd as administrator first.

If you still think it's a GSmartControl issue, please collect the following
information about your system. Without it, it may be very hard or impossible to fix the bug.

- Which operating system you use (for example, openSUSE Leap 15.3).
- Which version of GTK and Gtkmm you have installed. Finding this out is very
distribution-specific. For example, on openSUSE it would be `rpm -q gtk3 gtkmm3`.
Some distributions have `gtkmm30` instead. You may also search them in your 
distribution's graphical package manager, if there is one.
- Execution log from the program, if possible. To obtain it, run the program
with `-v` option, e.g. (type the following in a terminal emulator or Run dialog):
    ```
    gsmartcontrol-root -v
    ```
- Perform the steps needed to reproduce the bug, then go to
"Options -> View Execution Log", and click "Save All".
**Note:** On Windows, `-v` switch is on by default.
- Detailed description of steps you performed when the bug occurred.

Once you have this information, please open an issue on
[GitHub's Issue Tracker](https://github.com/ashaduri/gsmartcontrol/issues).
I may refer you to smartmontools support if it's a bug in smartmontools 
and not GSmartControl.

## Contact
You may contact me (Alexander Shaduri) directly at [ashaduri@gmail.com](mailto:ashaduri@gmail.com).


## Troubleshooting

### Known Issues

- Only ATA drives (both PATA and SATA), various USB to ATA bridges
and drives behind some RAID controllers are supported for now.
The main reasons for this are:
  - We can't support drives which don't work with smartmontools.
  This affects drives which don't support SMART or don't export SMART data
  correctly (e.g. some USB enclosures, RAIDs, etc...).
  - Smartctl's output for SCSI drives is completely different compared to ATA.
  Also, SCSI drives are rarely found in desktop systems and the servers rarely
  have X11 / Gtkmm running, so this is a low priority task.
- Immediate Offline Tests are not supported. I haven't found a way to reliably
monitor them. Besides, they run automatically anyway if Automatic Offline
Data Collection is enabled.
- Testing is only supported on drives which correctly report their progress
information in capabilities.
- Not all drives support disabling Automatic Offline Data Collection, even
if they report otherwise. Unfortunately, there's no way to detect such drives.

### Smartctl Options
GSmartControl tries its best to guard the user from having to specify smartctl options.
However, this is not always possible due to drive firmware bugs, unimplemented
features, and so on. The
[smartctl manual page](https://www.smartmontools.org/browser/trunk/smartmontools/smartctl.8.in)
contains all the information you may need when dealing with smartctl.

Additional information is available at [smartmontools.org](https://smartmontools.org)

### Permission Problems
You need to be root/Administrator to perform anything useful with GSmartControl.
This is needed because most operating systems prohibit direct access to
hardware to users with non-administrative privileges.

In Windows, UAC is automatically invoked when you run it. In other operating
systems, running gsmartcontrol-root (or using the desktop icon) will
automatically launch gsmartcontrol using the system's preferred su
mechanism - PolKit, kdesu, gnomesu, etc...

Please don't set the "setuid" flag on smartctl binary. It is considered
a security risk.
