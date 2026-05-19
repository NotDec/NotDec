#!/bin/bash

git push

pushd external/NotDec-llvm2c
git push
popd

pushd external/NotDec-wasm2llvm
git push
popd

pushd external/NotDec-evm2llvm
git push
popd

pushd /sn640/gigahorse-toolchain
git push
popd

pushd external/NotDec-bin2llvm
git push
popd

pushd external/binarysub
git push
popd
