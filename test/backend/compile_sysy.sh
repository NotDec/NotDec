#!/bin/bash

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
CLANG=/opt/wasi-sdk-20.0/bin/clang
NOTDEC_BIN=$SCRIPTPATH/../../build/bin/notdec

mkdir -p out_sysy_functional

set -e

SYSY=$SCRIPTPATH/../wasm/sysy/functional
for file in $SYSY/*.c; do
    name=$(basename $file)
    echo =========== $name ===========
    out=$SCRIPTPATH/out_sysy_functional/${name}
    /opt/wasi-sdk-20.0/bin/clang -Xclang -no-opaque-pointers -I$SYSY/.. -c -emit-llvm -S -o ${out}.ll $file
    echo $NOTDEC_BIN -i ${out}.ll -o ${out}.c
    $NOTDEC_BIN -i ${out}.ll -o ${out}.c
done
