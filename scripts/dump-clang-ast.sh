#!/bin/bash

clang -Wdocumentation -fparse-all-comments -Xclang -ast-dump -fsyntax-only $1
