#!/bin/sh

# Build Web Assembly version of Signaletic
./cross-build-wasm.sh

# Cross-compile non-native host examples
./cross-build-arm.sh
