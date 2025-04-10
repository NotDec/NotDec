#!/bin/bash

export NOTDEC_SUMMARY_OVERRIDE=/sn640/NotDec/experiment/dataset/summarys.json
export NOTDEC_SIGNATURE_OVERRIDE=/sn640/NotDec/experiment/dataset/signatures.json
build/bin/notdec $1 -o $1.c --debug-only=instcombine --print-after-all > $1.out 2>&1
