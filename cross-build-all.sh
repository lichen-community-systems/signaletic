#!/bin/sh

# Build wasm version of Signaletic
cd libsignaletic
./build-libsignaletic-wasm.sh

# Cross-compile non-native host examples
echo "\nCompiling Daisy Examples"
cd ../hosts/daisy/examples/bluemchen
make
