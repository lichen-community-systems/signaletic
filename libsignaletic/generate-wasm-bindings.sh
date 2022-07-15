#!/bin/sh

mkdir -p build/bindings

$EMSDK/upstream/emscripten/tools/webidl_binder wasm/bindings/libsignaletic-web-bindings.idl build/bindings/libsignaletic-web-bindings
