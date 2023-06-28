#!/bin/bash

set -e

BASEDIR=$(realpath $(dirname "$0"))

dataset_dir="$BASEDIR/dataset-SAC-2022"


mkdir -p $dataset_dir/build
pushd $dataset_dir/src
ls CWE*.c | xargs -I {} -n 1 -P $(nproc) bash -c ' \
/opt/wasi-sdk-16.0/bin/clang -I. -g -O0 --no-standard-libraries -Wl,--no-entry  -Wl,--allow-undefined -c -o ../build/{}.wasm {} && \
wasm2wat ../build/{}.wasm -o ../build/{}.wat
'
popd

# for file in $dataset_dir/src/*.c; do
#     name=$(basename $file)
#     echo compiling $name ...
#     out=$dataset_dir/out_functional/${name}
#     /opt/wasi-sdk-16.0/bin/clang -I. -g -O0 --no-standard-libraries -Wl,--no-entry  -Wl,--allow-undefined -c -o ${out}.wasm $file
#     wasm2wat ${out}.wasm -o ${out}.wat
# done
