#!/bin/bash

cmake -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/clang-14 -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++-14 -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld "-DCMAKE_CXX_FLAGS=-fdebug-default-version=4 -idirafter /usr/lib/llvm-14/lib/clang/14.0.0/include" --no-warn-unused-cli -S /sn640/NotDec -B /sn640/NotDec/build-release -G Ninja
cmake --build /sn640/NotDec/build-release
