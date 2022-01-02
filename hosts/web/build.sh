# TODO: Figure out how to get Meson to produce .wasm and .js output
# for libstar itself (not just executable targets).
mkdir -p build
emcc --no-entry -I../../libstar/include -I../../libstar/vendor/tlsf/ -o build/libstar.wasm ../../libstar/vendor/tlsf/tlsf.c ../../libstar/src/libstar.c -o build/libstar.js -s EXPORT_ALL=1 -s LINKABLE=1 -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'
