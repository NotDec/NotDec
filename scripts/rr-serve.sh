#!/bin/bash

export NOTDEC_SUMMARY_OVERRIDE=/sn640/NotDec/experiment/dataset/summarys.json
export NOTDEC_SIGNATURE_OVERRIDE=/sn640/NotDec/experiment/dataset/signatures.json

WORK_DIR=`mktemp -d`
# check if tmp dir was created
if [[ ! "$WORK_DIR" || ! -d "$WORK_DIR" ]]; then
  echo "Could not create temp dir"
  exit 1
fi

# deletes the temp directory
function cleanup {      
  rm -rf "$WORK_DIR"
  echo "Deleted temp working directory $WORK_DIR"
}

# register the cleanup function to be called on the EXIT signal
trap cleanup EXIT

cd $WORK_DIR
# gdbserver localhost:12345 /sn640/NotDec/build/bin/notdec $1 -o $1.c
rr record --no-file-cloning --no-read-cloning --asan /sn640/NotDec/build/bin/notdec $1 -o $1.c < /dev/null
rr replay -s 12345
