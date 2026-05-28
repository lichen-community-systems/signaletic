#!/bin/sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build/native"
SOURCE_DIR="$SCRIPT_DIR"

# If the build directory already exists, reconfigure to handle
# Meson version upgrades and other stale-state issues.
if [ -d "$BUILD_DIR" ]; then
    meson setup --reconfigure "$BUILD_DIR" "$SOURCE_DIR"
else
    meson setup "$BUILD_DIR" "$SOURCE_DIR"
fi

meson compile -C "$BUILD_DIR"
echo "Build complete."
