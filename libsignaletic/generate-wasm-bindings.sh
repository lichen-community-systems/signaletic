#!/bin/sh

mkdir -p build/bindings

$EMSCRIPTEN_TOOLS_PATH/webidl_binder wasm/bindings/libsignaletic-web-bindings.idl build/bindings/libsignaletic-web-bindings
