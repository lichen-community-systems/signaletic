#!/bin/sh

cd libsignaletic

# Run wasm tests
meson test -C build/wasm -v
