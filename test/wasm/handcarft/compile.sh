#!/bin/bash

set -e

BASEDIR=$(realpath $(dirname "$0"))

# if [ ! -f $BASEDIR/sylib.ll ]; then
#     /opt/wasi-sdk/wasi-sdk-16.0/bin/clang -target wasm32-unknown-wasi -S -emit-llvm ./sylib.c -O1 -o ./sylib.ll
# fi

mkdir -p out_functional
for file in $BASEDIR/functional/*.c; do
    name=$(basename $file)
    echo compiling $name ...
    out=$BASEDIR/out_functional/${name}
    /opt/wasi-sdk/wasi-sdk-16.0/bin/clang -I. -msimd128 --no-standard-libraries -Wl,--no-entry -lc -Wl,--allow-undefined -o ${out}.wasm $file
    wasm2wat ${out}.wasm -o ${out}.wat
done
