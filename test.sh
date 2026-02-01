#!/bin/bash

set -xe

export NOTDEC_SUMMARY_OVERRIDE=/sn640/NotDec/experiment/dataset/summarys.json
export NOTDEC_SIGNATURE_OVERRIDE=/sn640/NotDec/experiment/dataset/signatures.json

./build/bin/notdec /sn640/NotDec/experiment/dataset/out_dataset-SAC-2022/src/CWE124_Buffer_Underwrite__malloc_wchar_t_memcpy_15.wat -o /tmp/tmp.c
./build/bin/notdec --only-opt /sn640/NotDec/experiment/dataset/out_dataset-SAC-2022/src/CWE124_Buffer_Underwrite__malloc_wchar_t_memcpy_15.wat -o /tmp/tmp.c

./build/bin/GraphTest
