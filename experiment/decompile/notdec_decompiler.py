
import re
import os
import sys

if len(sys.argv) != 2:
    print("usage: notdec_decompiler.py src.wasm")
    exit()
cwd = os.path.dirname(os.path.realpath(__file__))
filename = os.path.basename(sys.argv[1])
filename_notsuffix = filename.split('.')[0]
filepath = os.path.abspath(sys.argv[1]).replace(filename, '')
ll_file = filepath + filename_notsuffix + '_notdec.ll'
code_file = filepath + filename_notsuffix + '_notdec.c'


def do_decompile(wasm,ll):
    command = [
        f"{cwd}/../../build/bin/notdec",
        "-i",
        wasm,
        "-o",
        ll,
        "--compat-mode",
        "--recompile",
    ]
    print(" ".join(command))
    return os.system(" ".join(command)) == 0


do_decompile(filepath+filename, ll_file)
ret = os.system(f"{cwd}/../bin/retdec-llvmir2hll {ll_file} -target-hll=c -o {code_file}")
if ret == 0:
    print(f'decompile output: {code_file}')
