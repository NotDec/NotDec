#!/usr/bin/env python3
"""Dump one function's block ids / successors, mirroring LLVMFunctionCFGBuilder:
BlockId = index of the basic block in IR order."""
import sys, importlib.util, re
spec = importlib.util.spec_from_file_location('ia', '/sn640/NotDec/logs/artifacts/20260922-structurer-duplication/ir_analyze.py')
ia = importlib.util.module_from_spec(spec); spec.loader.exec_module(ia)

path, key = sys.argv[1], sys.argv[2]
funcs = ia.split_functions(path)
for name, bbs in funcs.items():
    if key not in name: continue
    order = [l for l, _ in bbs]
    idx = {l: i for i, l in enumerate(order)}
    print('=== %s (%d blocks)' % (name, len(bbs)))
    for i, (lbl, instrs) in enumerate(bbs):
        succ = [s for s in ia.successors(instrs) if s in idx]
        term = [x.strip() for x in instrs if x.strip().startswith(('br ','switch','ret','unreachable','indirectbr'))]
        # shorten terminator
        t = term[-1] if term else '?'
        t = re.sub(r', !notdec.*$', '', t)
        first = next((x.strip() for x in instrs if x.strip()), '')
        print('  [%2d] %-26s -> %-22s | %s' % (i, lbl, ','.join(str(idx[s]) for s in succ), t[:100]))
