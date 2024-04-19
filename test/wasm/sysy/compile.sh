#!/bin/bash

set -e

BASEDIR=$(realpath $(dirname "$0"))
CLANG=/opt/wasi-sdk-20.0/bin/clang

if [ ! -f $BASEDIR/sylib.ll ]; then
    clang-14 -opaque-pointers=0 -S -emit-llvm ./sylib.c -O1 -o ./sylib.ll
fi

mkdir -p out_functional
for file in $BASEDIR/functional/*.c; do
    name=$(basename $file)
    echo compiling $name ...
    out=$BASEDIR/out_functional/${name}
    # /opt/wasi-sdk-16.0/bin/clang -I. -g -O0 --no-standard-libraries -Wl,--no-entry -lc -Wl,--allow-undefined -o ${out}.wasm $file
    # -Wl,--export-all   -Wl,--export=main
    $CLANG -fno-builtin -fno-lto -Wl,--lto-O0 -I. -g -O0 --no-standard-libraries -Wl,--entry=main -lc -Wl,--allow-undefined -o ${out}.wasm $file
    wasm2wat ${out}.wasm -o ${out}.wat
    # /opt/wasi-sdk-20.0/bin/clang -Xclang -no-opaque-pointers -I. -c -emit-llvm -S -o ${out}.ll $file
done
