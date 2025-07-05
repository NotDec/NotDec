#!/bin/bash

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"


for i in `ls $SCRIPTPATH/wasm`
do
  $SCRIPTPATH/../../../build/bin/notdec $SCRIPTPATH/wasm/$i -o /tmp/tmp.c 2>&1 | grep ConstraintGraph::saturate
done
