#!/bin/sh

mkdir -p build/bindings

$EMSCRIPTEN_TOOLS_PATH/webidl_binder wasm/bindings/libstar-web-bindings.idl build/bindings/libstar-web-bindings
