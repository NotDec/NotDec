#!/bin/bash

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

if [ -z "${1}" ]; then
    echo no arg 1: temp dir.
    exit
fi

souffle $SCRIPTPATH/pointer_main.dl --debug-report=$1/debug.html -F $1 -D $1
