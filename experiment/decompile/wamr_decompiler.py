import re
import os
import sys

if len(sys.argv) != 2:
    print("usage: wamr_decompiler.py src.wasm")
    exit()

cwd = os.path.dirname(os.path.realpath(__file__))
filename = os.path.basename(sys.argv[1])
filename_notsuffix = filename.split(".")[0]
filepath = os.path.abspath(sys.argv[1]).replace(filename, "")
ll_file = filepath + filename_notsuffix + "_wamr.ll"
fixll_file = filepath + filename_notsuffix + "_wamr_fix.ll"
code_file = filepath + filename_notsuffix + "_wamr.c"

os.system(
    f"{cwd}/../bin/wamrc --target=i386 --bounds-checks=0 --format=llvmir-opt -o {ll_file} {filepath + filename}"
)

ir = open(ll_file).read()
# remove attributes
ir = ir[: ir.index("attributes")]


# rename label
def rename(match) -> str:
    s = match.group()
    return "%a" + s[1:]


n = re.compile(r"%\d+")
ir = n.sub(rename, ir)
open(fixll_file, "w").write(ir)


# retdec
ret = os.system(f"{cwd}/../bin/retdec-llvmir2hll {fixll_file} -target-hll=c -o {code_file}")
if ret == 0:
    print(f'decompile output: {code_file}')
