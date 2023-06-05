#!/bin/bash

set -e

BASEDIR=$(realpath $(dirname "$0"))

dataset_dir="$BASEDIR/dataset-SAC-2022"


mkdir -p $dataset_dir/out_functional
for file in $dataset_dir/src/*.c; do
    name=$(basename $file)
    echo compiling $name ...
    out=$dataset_dir/out_functional/${name}
    /opt/wasi-sdk/wasi-sdk-16.0/bin/clang -I. -g -O0 --no-standard-libraries -Wl,--no-entry  -Wl,--allow-undefined -c -o ${out}.wasm $file
    wasm2wat ${out}.wasm -o ${out}.wat
done
