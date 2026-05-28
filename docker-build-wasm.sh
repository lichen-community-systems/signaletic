#!/bin/sh

# Build the Web Assembly version of Signaletic using a Docker container
# that has all necessary dependencies installed.
docker run -v `pwd`:/signaletic --rm signaletic /signaletic/cross-build-wasm.sh
