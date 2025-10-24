#!/bin/bash

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

cd $SCRIPTPATH

rm -r debug_dir
export NOTDEC_DEBUG_DIR=debug_dir
export NOTDEC_TYPE_RECOVERY_DEBUG_DIR=debug_dir
export NOTDEC_SUMMARY_OVERRIDE=/sn640/NotDec/experiment/dataset/summarys.json
export NOTDEC_SIGNATURE_OVERRIDE=/sn640/NotDec/experiment/dataset/signatures.json

# time perf record 
$SCRIPTPATH/build/bin/notdec $1 -o $1.c --tr-level=3
