#!/bin/bash

set -e

BASEDIR=$(realpath $(dirname "$0"))

mkdir -p out_functional
for file in $BASEDIR/functional/*.c; do
    name=$(basename $file)
    echo compiling $name ...
    out=$BASEDIR/out_functional/${name}
    /opt/wasi-sdk/wasi-sdk-16.0/bin/clang -I. -c -O -o ${out}.wasm $file
    wasm2wat ${out}.wasm -o ${out}.wat
done


