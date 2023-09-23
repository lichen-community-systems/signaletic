#!/bin/sh

# Build the Web Assembly version of Signaletic using a Docker container
# that has all the appropriate dependencies installed.
docker run -v `pwd`:/signaletic signaletic /signaletic/cross-build-wasm.sh
