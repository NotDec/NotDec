import re
import os
import sys

if sys.argv.count != 2:
    print("usage: wamr_decompiler.py src.wasm")

filename = os.path.basename(sys.argv[1])
ll_file = filename.split('.')[0] + '.ll'
fixll_file = filename.split('.')[0] + '_fix.ll'
code_file = filename.split('.')[0] + '.c'

os.system(f"./bin/wamrc --target=i386 --bounds-checks=0 --format=llvmir-opt -o {ll_file} {sys.argv[1]}")

ir = open(ll_file).read()
#remove attributes
ir = ir[:ir.index('attributes')]

#rename label
def rename(match) -> str:
    s = match.group()
    return '%a'+s[1:]

n = re.compile(r'%\d+')
ir = n.sub(rename, ir)
open(fixll_file, 'w').write(ir)


# retdec
os.system(f"./bin/retdec-llvmir2hll {fixll_file} -target-hll=c -o {code_file}")
