#!/bin/sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Remove all native build artifacts.
rm -rf "$SCRIPT_DIR/build/native"
echo "Cleaned native build artifacts."
