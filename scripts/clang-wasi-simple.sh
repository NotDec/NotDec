#!/bin/bash

CLANG=/opt/wasi-sdk-20.0/bin/clang

$CLANG -fno-builtin -fno-lto -Wl,--lto-O0 -I. -g -O0 --no-standard-libraries -Wl,--entry=main -lc -Wl,--allow-undefined $*
