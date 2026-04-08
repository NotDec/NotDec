#!/bin/bash

# 1. cd to dir that contains .sy files
# 2. execute scrite without changing current dir: `../add_include.sh`

for filename in ./*.sy; do
    sed -i '1 i\#include "sylib.h"' ./${filename}
    mv ./${filename} ./${filename%.sy}.c
done
