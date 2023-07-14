#!/bin/bash
rm ./out_dataset-SAC-2022/src/CWE*.dec.ll
rm ./out_dataset-SAC-2022/src/CWE*.dec.ll.svf
make -j`nproc` && python ../SVF/test_compile.py
