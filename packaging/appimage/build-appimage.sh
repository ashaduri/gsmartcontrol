#!/bin/bash
###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2025 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

# Script to build GSmartControl AppImage
# This script should be run from the repository root directory

set -e  # Exit on error
set -u  # Exit on undefined variable

# Configuration
APPDIR="${APPDIR:-AppDir}"
BUILD_DIR="${BUILD_DIR:-build-appimage}"
ARCH="${ARCH:-x86_64}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building GSmartControl AppImage${NC}"
echo "Architecture: $ARCH"
echo "Build directory: $BUILD_DIR"
echo "AppDir: $APPDIR"
echo

# Check dependencies
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: cmake not found. Please install cmake.${NC}"
    exit 1
fi

if ! command -v linuxdeploy &> /dev/null; then
    echo -e "${YELLOW}Warning: linuxdeploy not found. Will attempt to download it.${NC}"
    # Note: Using 'continuous' release for latest version. For reproducible builds,
    # pin to a specific release tag and verify SHA256 checksum.
    LINUXDEPLOY_URL="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage"
    wget -N "$LINUXDEPLOY_URL" -O linuxdeploy
    chmod +x linuxdeploy
    LINUXDEPLOY="./linuxdeploy"
else
    LINUXDEPLOY="linuxdeploy"
fi

if ! command -v appimagetool &> /dev/null; then
    echo -e "${YELLOW}Warning: appimagetool not found. Will attempt to download it.${NC}"
    # Note: Using 'continuous' release for latest version. For reproducible builds,
    # pin to a specific release tag and verify SHA256 checksum.
    APPIMAGETOOL_URL="https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-${ARCH}.AppImage"
    wget -N "$APPIMAGETOOL_URL" -O appimagetool
    chmod +x appimagetool
    APPIMAGETOOL="./appimagetool"
else
    APPIMAGETOOL="appimagetool"
fi

# Clean previous build
echo -e "${GREEN}Cleaning previous build...${NC}"
rm -rf "$BUILD_DIR"
rm -rf "$APPDIR"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake for AppImage build
echo -e "${GREEN}Configuring CMake for AppImage build...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DAPP_BUILD_APPIMAGE=ON \
    -DCMAKE_TOOLCHAIN_FILE="../toolchains/linux-dev.cmake"

# Build
echo -e "${GREEN}Building...${NC}"
cmake --build . --config RelWithDebInfo -j$(nproc)

# Install to AppDir
echo -e "${GREEN}Installing to AppDir...${NC}"
DESTDIR="../$APPDIR" cmake --install .

cd ..

# Copy desktop file and icon to AppDir root (required by AppImage)
echo -e "${GREEN}Setting up AppDir structure...${NC}"
cp "$APPDIR/usr/share/applications/dev.shaduri.gsmartcontrol.desktop" "$APPDIR/"
cp "$APPDIR/usr/share/icons/hicolor/256x256/apps/gsmartcontrol.png" "$APPDIR/"

# Create AppRun script
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
# AppRun script for GSmartControl AppImage

SELF=$(readlink -f "$0")
HERE=${SELF%/*}

# Export library paths
export LD_LIBRARY_PATH="$HERE/usr/lib:${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export PATH="$HERE/usr/bin:${PATH:+:$PATH}"

# Export GTK settings
export GSETTINGS_SCHEMA_DIR="$HERE/usr/share/glib-2.0/schemas:${GSETTINGS_SCHEMA_DIR:+:$GSETTINGS_SCHEMA_DIR}"
export GDK_PIXBUF_MODULEDIR="$HERE/usr/lib/gdk-pixbuf-2.0/2.10.0/loaders"
export GDK_PIXBUF_MODULE_FILE="$HERE/usr/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"

# Run the application
exec "$HERE/usr/bin/gsmartcontrol" "$@"
EOF
chmod +x "$APPDIR/AppRun"

# Use linuxdeploy to bundle dependencies
echo -e "${GREEN}Bundling dependencies with linuxdeploy...${NC}"
$LINUXDEPLOY --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/gsmartcontrol" \
    --desktop-file "$APPDIR/dev.shaduri.gsmartcontrol.desktop" \
    --icon-file "$APPDIR/gsmartcontrol.png"

# Update GDK pixbuf cache
if [ -d "$APPDIR/usr/lib/gdk-pixbuf-2.0" ]; then
    echo -e "${GREEN}Updating GDK pixbuf cache...${NC}"
    gdk-pixbuf-query-loaders > "$APPDIR/usr/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache" 2>/dev/null || true
fi

# Get version from version.txt (extract quoted CMAKE_PROJECT_VERSION value)
VERSION=$(sed -n 's/.*CMAKE_PROJECT_VERSION[[:space:]]*"\([^"]*\)".*/\1/p' version.txt | head -n1)

# Create AppImage
echo -e "${GREEN}Creating AppImage...${NC}"
ARCH=$ARCH $APPIMAGETOOL "$APPDIR" "GSmartControl-${VERSION}-${ARCH}.AppImage"

echo
echo -e "${GREEN}AppImage created successfully!${NC}"
echo -e "Output: ${GREEN}GSmartControl-${VERSION}-${ARCH}.AppImage${NC}"
echo
echo -e "${YELLOW}Note: The AppImage must be run with sudo to access disk drives.${NC}"
echo -e "Example: ${GREEN}sudo ./GSmartControl-${VERSION}-${ARCH}.AppImage${NC}"
