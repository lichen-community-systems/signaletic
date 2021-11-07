mkdir -p build
clang --target=wasm32 --no-standard-libraries -Wl,--export-all -Wl,--no-entry -I../../libstar/include -o build/libstar.wasm ../../libstar/src/libstar.c
