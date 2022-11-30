#!/bin/sh

echo "\nGenerating Web Assembly Bindings"
mkdir -p build/bindings
$EMSDK/upstream/emscripten/tools/webidl_binder wasm/bindings/libsignaletic-web-bindings.idl build/bindings/libsignaletic-web-bindings

echo "\nCompiling Web Assembly"
if [ ! -d "build/wasm" ]
then
    meson setup build/wasm --cross-file=wasm-cross-compile.txt
fi

meson compile -C build/wasm
