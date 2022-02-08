---
title: "Usage"
permalink: /usage
---

# Usage

## Launching GSmartControl on Desktop

### Linux and Unix
On Linux and Unix systems, please use the desktop menu entry. Alternatively, you can
run `gsmartcontrol-root`, which invokes GSmartControl using your desktop's `su` mechanism.

### Windows
Simply install GSmartControl and run it from the Start menu.

### macOS
After being installed using Homebrew, GSmartControl can be run by
typing `gsmartcontrol` in Terminal. If all you get is `Command not found`, please run
a `brew doctor` command first.


## Command-Line Options

**Note:** The Windows version may not output any text to a command-line window,
so `--help` and similar options will be of no help.

The most important options are:

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


### Advanced Options

`-l`, `--no-locale` - Don't use system locale.

`--no-hide-tabs` - Don't hide non-identity tabs when SMART is disabled. Useful
for debugging.


## Smartctl Options

GSmartControl provides the ability to specify custom options to smartctl. The
[smartctl manual page](https://www.smartmontools.org/browser/trunk/smartmontools/smartctl.8.in)
contains detailed information on these options. Additional information is available
at [smartmontools.org](https://smartmontools.org).
