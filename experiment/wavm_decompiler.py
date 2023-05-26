import re
import os
import sys

if sys.argv.count != 2:
    print("usage: wavm_decompiler.py src.wasm")

filename = os.path.basename(sys.argv[1])
ll_file = filename.split('.')[0] + '.ll'
fixll_file = filename.split('.')[0] + '_fix.ll'
code_file = filename.split('.')[0] + '.c'

# wasm2llvmir
os.system(
    f"./bin/wavm compile --format=optimized-llvmir {sys.argv[1]} {ll_file} --target-cpu skylake")
ir = open(ll_file).read()

# remove llvm ir dbg info
p = re.compile(r'[ ,]*!dbg !\d+')
ir = p.sub('', ir)
ir = ir[:ir.index('!llvm.dbg.cu')]


# args rename
ar = re.compile(r'\(.*\) #')

def rename_arg(match) -> str:
    s = match.group()[:-3].split(',')
    d = ''
    for idx, v in enumerate(s):
        d += v + f' %a{idx},'
    return d[:-1] + ') #'


ir = ar.sub(rename_arg, ir)


# rename label
def rename(match) -> str:
    s = match.group()
    return '%a'+s[1:]


n = re.compile(r'%\d+')
ir = n.sub(rename, ir)


open(fixll_file, 'w').write(ir)
# retdec
os.system(f"./bin/retdec-llvmir2hll {fixll_file} -target-hll=c -o {code_file}")
