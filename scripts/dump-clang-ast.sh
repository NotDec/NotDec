#!/bin/bash

clang -Xclang -ast-dump -fsyntax-only $1
