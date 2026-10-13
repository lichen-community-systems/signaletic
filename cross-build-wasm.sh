#!/bin/sh
set -e

# Build Web Assembly version of Signaletic
cd libsignaletic
./build-libsignaletic-wasm.sh
cd ..
