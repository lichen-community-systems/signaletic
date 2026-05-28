#!/bin/sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Run the Signaletic unit tests natively.
# Ensure the library is built first (compile.sh).
meson test -C "$SCRIPT_DIR/build/native" -v
