#!/bin/sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Run wasm tests
node $SCRIPT_DIR/build/wasm/run_tests.js
