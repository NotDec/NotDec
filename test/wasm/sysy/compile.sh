#!/bin/bash

set -e

BASEDIR=$(realpath $(dirname "$0"))

if [ ! -f $BASEDIR/sylib.ll ]; then
    clang-13 -S -emit-llvm ./sylib.c -o ./sylib.ll
fi

mkdir -p out_functional
for file in $BASEDIR/functional/*.c; do
    name=$(basename $file)
    echo compiling $name ...
    out=$BASEDIR/out_functional/${name}
    /opt/wasi-sdk/wasi-sdk-16.0/bin/clang -I. -g -O1 --no-standard-libraries -Wl,--no-entry -Wl,--export-all -Wl,--allow-undefined -o ${out}.wasm $file
    wasm2wat ${out}.wasm -o ${out}.wat
done


