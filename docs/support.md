---
title: "Support"
permalink: /support
---

# Support


## Reporting Bugs

Please report issues at GSmartControl's
[Issue Tracker](https://github.com/ashaduri/gsmartcontrol/issues) on GitHub.


## Before Filing an Issue

Please see the [Troubleshooting](troubleshooting.md) page before reporting any issues.

If it is a SMART or drive-related problem, please try to test it with smartctl first.
Chances are, the problem you're experiencing is not tied to GSmartControl,
but is a drive firmware or smartctl problem. For example, to see complete
information about your `/dev/sda` drive, type the following in a terminal
emulator (as `root`, using `sudo` or `su`):
```
smartctl -x /dev/sda
```
**Note:** If using Windows, the device name should be `/dev/pd1` for the
second physical drive, etc. Run `cmd` as administrator first.

If you still think it's a GSmartControl issue, please collect the following
information about your system:

- Which operating system you use (for example, openSUSE Leap 15.5).
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


## Contact
You may contact me (Alexander Shaduri) directly at [ashaduri@gmail.com](mailto:ashaduri@gmail.com).

