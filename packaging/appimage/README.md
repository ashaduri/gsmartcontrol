# GSmartControl AppImage Build

This directory contains scripts and configurations for building GSmartControl as an AppImage.

## What is AppImage?

AppImage is a format for distributing portable software on Linux without requiring installation. AppImages are self-contained applications that can run on most Linux distributions.

## Building the AppImage

### Prerequisites

You need to have the following installed:
- cmake (>= 3.14)
- g++ or clang with C++20 support
- GTK3 and gtkmm-3.0 development packages
- smartmontools (for runtime)
- linuxdeploy and appimagetool (the script will download them if not available)

On Ubuntu/Debian:
```bash
sudo apt-get install cmake g++ libgtkmm-3.0-dev gettext smartmontools
```

On Fedora:
```bash
sudo dnf install cmake gcc-c++ gtkmm30-devel gettext smartmontools
```

### Build Steps

From the repository root directory, run:

```bash
./packaging/appimage/build-appimage.sh
```

The script will:
1. Configure CMake with `-DAPP_BUILD_APPIMAGE=ON`
2. Build the application
3. Create an AppDir with the proper structure
4. Bundle dependencies using linuxdeploy
5. Create the final AppImage file

### Output

The script creates an AppImage file named `GSmartControl-<version>-<arch>.AppImage` in the repository root.

## Running the AppImage

**Important:** GSmartControl requires root privileges to access disk drives. You must run the AppImage with sudo:

```bash
sudo ./GSmartControl-<version>-x86_64.AppImage
```

### Why sudo is required?

- AppImage format does not support embedded privilege escalation scripts
- The binary must be executed directly as root to access `/dev/sd*` and `/dev/nvme*` devices
- This is different from traditional package installations which use the `gsmartcontrol-root` wrapper script

## AppImage-Specific Changes

When building with `-DAPP_BUILD_APPIMAGE=ON`, the following changes are applied:

1. **Data file paths**: Uses relative paths from binary location (`bin/../share/...`)
2. **Desktop file**:
   - Uses rDNS format: `dev.shaduri.gsmartcontrol.desktop`
   - Executes binary directly without `gsmartcontrol-root` wrapper
3. **Binary location**: Installed to `bin/` instead of `sbin/`
4. **No wrapper script**: `gsmartcontrol-root` is not installed

## Compatibility

The AppImage should work on:
- Fedora Atomic (Silverblue, Kinoite, etc.)
- Most modern Linux distributions with GTK3 support
- Systems with read-only root filesystems

Tested architectures:
- x86_64 (Intel/AMD 64-bit)

## Troubleshooting

### "Permission denied" when running AppImage
Make the AppImage executable:
```bash
chmod +x GSmartControl-*.AppImage
```

### "No drives detected"
Make sure to run with sudo:
```bash
sudo ./GSmartControl-*.AppImage
```

### GTK theme issues
You may need to install GTK3 themes on your system. The AppImage bundles the necessary libraries but uses system themes when available.

## Notes for Developers

- The AppImage build uses the same source code as regular builds
- The `BuildEnv::is_appimage_build()` function can be used to detect AppImage builds at runtime
- Icon and UI file paths are resolved at runtime relative to the binary location
- The AppImage includes all necessary GTK3 and gtkmm libraries
