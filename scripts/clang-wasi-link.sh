#!/bin/bash

CLANG=/opt/wasi-sdk-20.0/bin/clang

#  -lc
$CLANG -fno-builtin -fno-lto -Wl,--lto-O0 -I. -g -O0 --no-standard-libraries -Wl,--entry=main -Wl,--allow-undefined $*
