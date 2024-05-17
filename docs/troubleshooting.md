---
title: "Troubleshooting"
permalink: /troubleshooting
---

# Troubleshooting

Please see the [Support](support.md) page for information on how to report issues. 


## Known Limitations

- Only ATA drives (both PATA and SATA), NVMe drives, various USB to ATA
bridges, and drives behind some RAID controllers are supported for now.
The main reasons for this are:
  - We can't support drives which don't work with smartmontools.
  This affects drives which don't support SMART or don't export SMART data
  correctly (e.g. some USB enclosures, RAIDs, etc.).
  - SCSI drives are rarely found in desktop systems and the servers rarely
  have X11 / Gtkmm running, so this is a low priority task.
- Immediate Offline Tests are not supported due to being mostly obsolete.


## Custom Smartctl Options
GSmartControl tries its best to guard the user from having to specify smartctl options.
However, this is not always possible due to drive firmware bugs, unimplemented
features, and so on.

GSmartControl provides the ability to specify custom options to smartctl. The
[smartctl manual page](https://www.smartmontools.org/browser/trunk/smartmontools/smartctl.8.in)
contains detailed information on these options. Additional information is available
at [smartmontools.org](https://smartmontools.org).


## Permission Problems
You need to have root / Administrator privileges to perform anything useful with GSmartControl.
This is needed because most operating systems prohibit direct access to
hardware to users with non-administrative privileges.

In Windows, UAC is automatically invoked when you run it. In Linux / Unix operating
systems, running `gsmartcontrol-root` (or using the desktop icon) will
automatically launch GSmartControl using the system's preferred su
mechanism - `PolKit`, `kdesu`, `gnomesu`, etc.

Please **do not** set the `setuid` flag on smartctl binary. It is considered
a security risk.


## SMART Does Not Stay Enabled
Specifications say that once you set a SMART-related property, it will 
be preserved across reboots. For example, when you enable SMART and 
Automatic Offline Data Collection, both will stay enabled until you disable them.

However, BIOS / UEFI, your operating system, your other operating systems
(if present), and various startup programs may affect that. For example,
UEFI may enable SMART each time you start your computer, so if you 
disabled SMART previously, it will be re-enabled on reboot.

The easiest way to work around this is to set the desired settings on
system startup. You may use `smartctl` or `smartd` to do that. For example,
to enable both SMART and Automatic Offline Data Collection on `/dev/sda`,
one would write the following to the system startup script (e.g. `boot.local`,
`rc.local` or similar on Linux):
```
smartctl -s on -o on /dev/sda
```
For more information, see `smartctl` and `smartd` [documentation](https://smartmontools.org).
