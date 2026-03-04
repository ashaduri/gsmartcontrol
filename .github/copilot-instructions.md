# GSmartControl – Copilot Instructions

GSmartControl is a C++20 GTK3/Gtkmm GUI for inspecting hard-drive and SSD health via SMART data
(wrapping `smartctl` from smartmontools). It supports Linux, Windows (MSYS2/MinGW), macOS, and BSDs.

## Build

```bash
mkdir build && cd build

# Standard Linux build (uses dev toolchain with warnings enabled)
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo \
         -DCMAKE_TOOLCHAIN_FILE=../toolchains/linux-dev.cmake

# Windows (MSYS2/MinGW64)
cmake .. -G "MSYS Makefiles" \
         -DCMAKE_BUILD_TYPE=RelWithDebInfo \
         -DCMAKE_TOOLCHAIN_FILE=../toolchains/win64-mingw-msys2.cmake

# Build on MSYS2/MinGW64 (from build dir)
cmake --build . --target all -j 18
```

The `configure-dev` script is a convenience wrapper: `./configure-dev -t <toolchain> -c <compiler> -b <build_type>`.

Key CMake options:
- `-DAPP_BUILD_TESTS=ON` – enable test targets
- `-DAPP_BUILD_EXAMPLES=ON` – enable example targets
- `-DAPP_COMPILER_ENABLE_WARNINGS=ON` – strict compiler warnings (set automatically by `linux-dev.cmake`)

## Tests

Tests use **Catch2** (vendored in `dependencies/catch2/`). They are only built when `-DAPP_BUILD_TESTS=ON` is passed.

```bash
# Build and run all tests
cmake .. -DAPP_BUILD_TESTS=ON
cmake --build .
ctest -C RelWithDebInfo --rerun-failed --output-on-failure

# Run a single test executable directly (from build dir)
./test_all "[smartctl_parser]"        # Catch2 tag filter
./test_all "test name substring"      # substring match
```

Test sources live in `src/applib/tests/` and `src/hz/tests/`; they are linked into `src/test_all/`.

## Linting

Clang-tidy config is at `.clang-tidy` in the repo root. Run it via CMake or directly:

```bash
clang-tidy src/applib/storage_device.cpp -- -std=c++20 $(pkg-config --cflags gtkmm-3.0)
```

## Architecture

The codebase is split into libraries and one GUI executable:

| Component | Location | Purpose |
|---|---|---|
| `applib` | `src/applib/` | Core logic: SMART parsing, device detection, command execution |
| `hz` | `src/hz/` | General-purpose C++ utilities (strings, filesystem, error types) |
| `rconfig` | `src/rconfig/` | Runtime config load/save with auto-save support |
| `libdebug` | `src/libdebug/` | Debug logging with named channels |
| `gui` | `src/gui/` | GTK/Gtkmm UI (windows, dialogs, Glade `.ui` files) |

### Parsing pipeline

`smartctl` output is parsed through a strategy pattern:
- JSON parsers (`smartctl_json_ata_parser`, `smartctl_json_nvme_parser`, `smartctl_json_basic_parser`) target smartctl ≥ 7.3.
- Text parsers (`smartctl_text_ata_parser`, `smartctl_text_basic_parser`) handle legacy output.
- Parsed results are stored as `StorageProperty` objects in a `StoragePropertyRepository` on `StorageDevice`.

### Device detection

`StorageDetector` is an abstract interface; platform implementations are:
- `storage_detector_linux.cpp` – scans `/dev/sd*`, `/dev/nvme*`, etc.
- `storage_detector_win32.cpp` – WMI/registry enumeration
- `storage_detector_other.cpp` – generic `/dev` scanning (BSD/macOS)

### Async execution

`AsyncCommandExecutor` wraps `CommandExecutor` (which shells out to `smartctl`) in a background thread. 
Completion is signalled via **libsigc++** signals back to the GTK main loop. Never call GTK APIs from the worker thread;
queue them through signals or `Glib::signal_idle()`.

### Error handling

Use `hz::ExpectedValue<T, E>` (wrapping `tl::expected`) for recoverable errors instead of exceptions. 
Enum-based error types (`StorageDeviceError`, `SmartctlParserError`) are preferred over string errors.

## Key Conventions

**Naming:**
- Classes: `PascalCase` (e.g., `StorageDevice`, `SmartctlParser`)
- Methods and members: `snake_case`; private/member fields have a trailing `_` (e.g., `device_list_`)
- GUI classes in `src/gui/` are prefixed `Gsc` (e.g., `GscMainWindow`)
- Filenames: `snake_case.cpp` / `snake_case.h`

**Headers:**
- Use `#ifndef FILENAME_H` / `#define FILENAME_H` guards (not `#pragma once`)
- Include order: standard library → third-party (GTK/Gtkmm) → project headers
- All project headers are included relative to `src/` (the include root)

**Modern C++20/23 patterns used throughout:**
- `std::optional<T>` for nullable values
- `hz::ExpectedValue<T, E>` for error-returning functions
- `std::string_view` for non-owning string parameters
- `fmt::format()` (vendored `fmt` library) for string formatting
- Smart pointers exclusively; no raw owning pointers

**Vendored dependencies** are in `dependencies/` and added via `add_subdirectory`.
Do not modify them; prefer upgrading the whole vendored copy.

**Translations:** UI strings are wrapped with `_()` (gettext). `.po` files live in `po/`.
Translations are not yet supported.

**Platform guards:** Use `#ifdef CONFIG_KERNEL_FAMILY_WINDOWS`, 
`#ifdef CONFIG_KERNEL_LINUX`, etc.

## Agentic Development

Place all plans and temporary files in `agent_workspace/`. This directory is ignored by Git.
