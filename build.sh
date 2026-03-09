#!/usr/bin/env bash
set -e

# StickBrawl Linux Build Script
# Requires: cmake, g++ (or clang++), vcpkg

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
BUILD_TYPE="${1:-Release}"

# --- Locate vcpkg ---
if [ -n "$VCPKG_ROOT" ]; then
    VCPKG_TOOLCHAIN="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
elif [ -d "$HOME/vcpkg" ]; then
    VCPKG_TOOLCHAIN="$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake"
else
    echo "ERROR: Cannot find vcpkg."
    echo "  Set VCPKG_ROOT or install vcpkg to ~/vcpkg:"
    echo "    git clone https://github.com/microsoft/vcpkg.git ~/vcpkg"
    echo "    ~/vcpkg/bootstrap-vcpkg.sh"
    exit 1
fi

echo "=== StickBrawl Build ==="
echo "  Build type : $BUILD_TYPE"
echo "  Build dir  : $BUILD_DIR"
echo "  vcpkg      : $VCPKG_TOOLCHAIN"
echo

# --- Install system dependencies (Ubuntu/Debian) ---
if command -v apt-get &>/dev/null; then
    NEEDED_PKGS=""
    for pkg in libx11-dev libxi-dev libxrandr-dev libxcursor-dev libudev-dev libgl1-mesa-dev; do
        if ! dpkg -s "$pkg" &>/dev/null; then
            NEEDED_PKGS="$NEEDED_PKGS $pkg"
        fi
    done
    if [ -n "$NEEDED_PKGS" ]; then
        echo "--- Installing system packages:$NEEDED_PKGS ---"
        sudo apt-get install -y $NEEDED_PKGS
        echo
    fi
fi

# --- Install dependencies via vcpkg ---
echo "--- Installing dependencies via vcpkg ---"
VCPKG_BIN="$(dirname "$VCPKG_TOOLCHAIN")/../vcpkg"
if [ -x "$VCPKG_BIN" ]; then
    "$VCPKG_BIN" install sfml box2d nlohmann-json
else
    echo "Warning: vcpkg binary not found at $VCPKG_BIN, assuming deps are already installed."
fi
echo

# --- Configure ---
echo "--- Configuring (CMake) ---"
cmake -B "$BUILD_DIR" -S "$SCRIPT_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_TOOLCHAIN"
echo

# --- Build ---
echo "--- Building ---"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j "$(nproc)"
echo

echo "=== Build complete ==="
echo "  Run: $BUILD_DIR/StickBrawl"
