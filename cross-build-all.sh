#!/bin/sh

# Build Web Assembly version of Signaletic
cd libsignaletic
./build-libsignaletic-wasm.sh

# Cross-compile non-native host examples
cd ../hosts/daisy
./build-all-daisy.sh
