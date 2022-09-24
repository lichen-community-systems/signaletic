#!/bin/sh

echo "\nCompiling libDaisy"
cd vendor/libDaisy
make

echo "\nCompiling Bluemchen Examples"
cd ../../examples/bluemchen
make

echo "\nCompiling DPT Examples"
cd ../../examples/dpt
make
