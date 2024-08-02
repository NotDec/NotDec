#!/bin/bash

CLANG=/opt/wasi-sdk-20.0/bin/clang

/opt/wasi-sdk-16.0/bin/clang -I. -c -O0 $*
