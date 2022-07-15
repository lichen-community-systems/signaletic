#!/bin/sh

echo "\nCompiling libDaisy"
cd vendor/libDaisy
make

echo "\nCompiling Daisy Examples"
cd ../../examples/bluemchen
make
